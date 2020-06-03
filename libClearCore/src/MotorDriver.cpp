/*
 * Copyright (c) 2020 Teknic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
    Implementation of ClearCore ClearPath motor connector
**/

#include "MotorDriver.h"
#include <stdio.h>
#include <sam.h>
#include "atomic_utils.h"
#include "InputManager.h"
#include "StatusManager.h"
#include "SysTiming.h"
#include "SysUtils.h"

#define HLFB_CARRIER_LOSS_ERROR_LIMIT (0)
#define HLFB_CARRIER_LOSS_STATE_CHANGE_MS (25)
#define HLFB_CARRIER_LOSS_STATE_CHANGE_SAMPLES (MS_TO_SAMPLES * HLFB_CARRIER_LOSS_STATE_CHANGE_MS)

namespace ClearCore {

extern SysTiming &TimingMgr;
extern ShiftRegister ShiftReg;
extern volatile uint32_t tickCnt;

static Tcc *const tcc_modules[TCC_INST_NUM] = TCC_INSTS;

// Needed to calculate the correct index into the CCBUF register
// for a given TCC when constructing motor connectors.
uint8_t TccCcNum(uint8_t tccNum) {
    uint8_t tccCcNum;
    switch (tccNum) {
        case 0:
            tccCcNum = TCC0_CC_NUM;
            break;
        case 1:
            tccCcNum = TCC1_CC_NUM;
            break;
        case 2:
            tccCcNum = TCC2_CC_NUM;
            break;
        case 3:
            tccCcNum = TCC3_CC_NUM;
            break;
        case 4:
            tccCcNum = TCC4_CC_NUM;
            break;
        default:
            tccCcNum = 0;
            break;
    }
    return tccCcNum;
}

/*
    Construct and wire in our IO pins
*/
MotorDriver::MotorDriver(ShiftRegister::Masks enableMask,
                         const PeripheralRoute *aInfo,
                         const PeripheralRoute *bInfo,
                         const PeripheralRoute *hlfbInfo,
                         uint16_t hlfbTc,
                         uint16_t hlfbEvt)
    : DigitalIn(ShiftRegister::Masks::SR_NO_FEEDBACK_MASK, hlfbInfo),
      m_enableMask(enableMask),
      m_aInfo(aInfo),
      m_bInfo(bInfo),
      m_hlfbInfo(hlfbInfo),
      m_aDataMask(1UL << aInfo->gpioPin),
      m_bDataMask(1UL << bInfo->gpioPin),
      m_hlfbTcNum(hlfbTc),
      m_hlfbEvt(hlfbEvt),
      m_hlfbMode(HLFB_MODE_STATIC),
      m_hlfbNoPwmSampleCount(2),
      m_hlfbDuty(HLFB_DUTY_UNKNOWN),
      m_hlfbState(HLFB_UNKNOWN),
      m_hlfbPwmReadingPending(false),
      m_hlfbStateChangeCounter(0),
      m_polarityInversions(0),
      m_enableTriggerActive(false),
      m_enableTriggerPulseStartMs(0),
      m_enableTriggerPulseCount(0),
      m_enableTriggerPulseLenMs(25),
      m_aDutyCnt(0),
      m_bDutyCnt(0),
      m_inFault(false),
      m_enableRequestedState(false),
      m_statusRegMotor(0),
      m_statusRegMotorRisen(0),
      m_statusRegMotorFallen(0),
      m_initialized(false),
      m_isEnabling(false),
      m_isEnabled(false),
      m_hlfbCarrierLost(false),
      m_enableCounter(0) {

    m_interruptAvail = true;
    m_aTccBuffer =
        &(tcc_modules[m_aInfo->tccNum])->CCBUF[m_aInfo->tccPadNum %
                TccCcNum(m_aInfo->tccNum)].reg;
    m_bTccBuffer =
        &(tcc_modules[m_bInfo->tccNum])->CCBUF[m_bInfo->tccPadNum %
                TccCcNum(m_bInfo->tccNum)].reg;

    for (uint8_t i = 0; i < CPM_HLFB_CAP_HISTORY; i++) {
        m_hlfbWidth[i] = 0;
        m_hlfbPeriod[i] = 0;
    }
}

/*
    Update the HLFB state
*/
void MotorDriver::Refresh() {
    if (!m_initialized) {
        return;
    }

    // Process the HLFB input as static levels/filtering
    DigitalIn::Refresh();

    // Pointer to our TC
    static Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;
    TcCount16 *tcCount = &tc_modules[m_hlfbTcNum]->COUNT16;
    uint8_t intFlagReg = tcCount->INTFLAG.reg;

    bool invert = (m_mode == CPM_MODE_STEP_AND_DIR) &&
                   m_polarityInversions.bit.hlfbInverted;

    // Process the HLFB information
    switch (m_hlfbMode) {
        HlfbStates readHlfbState;
        case HLFB_MODE_HAS_PWM:
        case HLFB_MODE_HAS_BIPOLAR_PWM:
            // Check for overflow or error conditions
            if ((intFlagReg & TC_INTFLAG_OVF) || 
                (intFlagReg & TC_INTFLAG_ERR)) {
                tcCount->INTFLAG.reg = TC_INTFLAG_OVF | TC_INTFLAG_MC0 |
                                       TC_INTFLAG_ERR | TC_INTFLAG_MC1;
                // Saturating increment
                m_hlfbNoPwmSampleCount = __QADD16(m_hlfbNoPwmSampleCount, 1U);
                m_hlfbCarrierLost =
                    m_hlfbNoPwmSampleCount > HLFB_CARRIER_LOSS_ERROR_LIMIT;
            }
            // Did we capture a period?
            else if (intFlagReg & TC_INTFLAG_MC0) {
                if (m_hlfbNoPwmSampleCount) {
                    // When coming out of overflow/error conditions do not use
                    // the next measurement because the pulse may be clipped
                    tcCount->INTFLAG.reg = TC_INTFLAG_MC0 | TC_INTFLAG_MC1;
                    m_hlfbPwmReadingPending = false;
                    m_hlfbNoPwmSampleCount = 0;
                }
                else if (intFlagReg & TC_INTFLAG_MC1) {

                    // Save history and use only the (n-1)st item to return as the
                    // last PWM captured might be clipped in a move done case.
                    m_hlfbWidth[0] = m_hlfbWidth[CPM_HLFB_CAP_HISTORY - 1];
                    m_hlfbPeriod[0] = m_hlfbPeriod[CPM_HLFB_CAP_HISTORY - 1];
                    m_hlfbWidth[CPM_HLFB_CAP_HISTORY - 1] = tcCount->CC[1].reg;
                    m_hlfbPeriod[CPM_HLFB_CAP_HISTORY - 1] = tcCount->CC[0].reg;

                    // Both period and pulse width have been captured
                    // If this is the first valid pulse, wait until a second
                    // pulse is seen before announcing that a measurement is
                    // present. This ensures that the pulse isn't being clipped
                    // by having the signal go away.
                    if (m_hlfbPwmReadingPending) {
                        m_hlfbCarrierLost = false;
                        float dutyCycle = static_cast<float>(m_hlfbWidth[0]) /
                                          static_cast<float>(m_hlfbPeriod[0]);
                        // Convert to floating and inflate 5-95% to 0-100%
                        m_hlfbDuty = (dutyCycle - 0.05) * (10000. / 90.);

                        if (invert) {
                            m_hlfbDuty = 100 - m_hlfbDuty;
                        }

                        // Convert unipolar to bipolar?
                        if (m_hlfbMode == HLFB_MODE_HAS_BIPOLAR_PWM) {
                            m_hlfbDuty = 2.0 * (m_hlfbDuty - 50.);
                        }
                        m_hlfbState = HLFB_HAS_MEASUREMENT;
                    }
                    m_hlfbPwmReadingPending = true;
                }
            }
            if (!m_hlfbCarrierLost) {
                break;
            }
            else {
                // check for an HLFB state change
                readHlfbState = (DigitalIn::m_stateFiltered ^ invert) ?
                HLFB_ASSERTED : HLFB_DEASSERTED;
                if (readHlfbState != m_hlfbState &&
                m_hlfbStateChangeCounter++ < HLFB_CARRIER_LOSS_STATE_CHANGE_SAMPLES) {
                    break;
                }
                else {
                    m_hlfbStateChangeCounter = 0;
                }
            }

        // Fall through to process as a static signal if the carrier is lost
        case HLFB_MODE_STATIC:
        default:
            m_hlfbDuty = HLFB_DUTY_UNKNOWN;
            m_hlfbState = (DigitalIn::m_stateFiltered ^ invert) ?
                          HLFB_ASSERTED : HLFB_DEASSERTED;
            break;
    }

    // Update the Motor Status Register
    StatusRegMotor statusRegLast = m_statusRegMotor;
    StatusRegMotor statusRegPending = m_statusRegMotor;
    statusRegPending.bit.Triggering = m_enableTriggerActive;
    statusRegPending.bit.MoveDirection = StepGenerator::m_direction;
    statusRegPending.bit.StepsActive =
        (StepGenerator::m_moveState != StepGenerator::MOVE_STATES::MS_IDLE &&
        StepGenerator::m_moveState != StepGenerator::MOVE_STATES::MS_END);
    statusRegPending.bit.AtVelTarget = 
        (StepGenerator::m_moveState == StepGenerator::MOVE_STATES::MS_CRUISE);

    if (m_isEnabling) {
        if (m_enableCounter > 0) {
            m_enableCounter--;
        }
        else {
            m_isEnabled = true;
            m_isEnabling = false;
        }
    }
    statusRegPending.bit.Enabled = m_isEnabled;

    if (!(m_isEnabled || m_isEnabling)) {
        statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_DISABLED;
    }
    else if (m_isEnabling) {
        statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_ENABLING;
    }
    else {
        if ((m_hlfbMode == HLFB_MODE_STATIC &&
                m_hlfbState == MotorDriver::HlfbStates::HLFB_DEASSERTED) ||
                statusRegPending.bit.StepsActive) {
            statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_MOVING;
        }
        else if (m_hlfbMode != HLFB_MODE_STATIC &&
                 m_hlfbState == MotorDriver::HlfbStates::HLFB_DEASSERTED) {
            statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_FAULTED;
        }
        else {
            statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_READY;
        }
    }

    m_statusRegMotor = statusRegPending;

    m_statusRegMotorRisen.reg = (~statusRegLast.reg & statusRegPending.reg) |
                                m_statusRegMotorRisen.reg;
    m_statusRegMotorFallen.reg = (statusRegLast.reg & ~statusRegPending.reg) |
                                 m_statusRegMotorFallen.reg;

    // Calculate the next S&D output step count
    if (Connector::m_mode == Connector::CPM_MODE_STEP_AND_DIR) {
        // Queue up the steps calculated in the previous sample by
        // writing the B duty value
        UpdateBDuty();
        // Calculate the number of steps to send in the next sample time
        StepGenerator::StepsCalculated();
        m_bDutyCnt = StepGenerator::m_stepsPrevious;
    }
}

MotorDriver::StatusRegMotor MotorDriver::StatusRegRisen() {
    return StatusRegMotor(atomic_exchange_n(&m_statusRegMotorRisen.reg, 0));
}

MotorDriver::StatusRegMotor MotorDriver::StatusRegFallen() {
    return StatusRegMotor(atomic_exchange_n(&m_statusRegMotorFallen.reg, 0));
}

bool MotorDriver::MotorInAState() {
    bool retVal = !(PORT->Group[m_aInfo->gpioPort].OUT.reg & m_aDataMask);
    if (Connector::m_mode == Connector::CPM_MODE_STEP_AND_DIR &&
            m_polarityInversions.bit.directionInverted) {
        retVal = !retVal;
    }

    return retVal;
}

bool MotorDriver::MotorInBState() {
    return !(PORT->Group[m_bInfo->gpioPort].OUT.reg & m_bDataMask);
}

bool MotorDriver::MotorInAState(bool value) {
    if (Connector::m_mode == Connector::CPM_MODE_STEP_AND_DIR ||
            Connector::m_mode == Connector::CPM_MODE_A_PWM_B_PWM) {
        return false;
    }

    DATA_OUTPUT_STATE(m_aInfo->gpioPort, m_aDataMask, !value);
    return true;
}

bool MotorDriver::MotorInBState(bool value) {
    if (Connector::m_mode != Connector::CPM_MODE_A_DIRECT_B_DIRECT) {
        return false;
    }

    DATA_OUTPUT_STATE(m_bInfo->gpioPort, m_bDataMask, !value);
    return true;
}

bool MotorDriver::MotorInADuty(uint8_t duty) {
    if (Connector::m_mode == Connector::CPM_MODE_A_PWM_B_PWM) {
        m_aDutyCnt = (static_cast<uint32_t>(duty) * m_stepsPerSampleMax +
                      (UINT8_MAX / 2)) / UINT8_MAX;
        UpdateADuty();
        return true;
    }
    return false;
}

bool MotorDriver::MotorInBDuty(uint8_t duty) {
    if (Connector::m_mode == Connector::CPM_MODE_A_DIRECT_B_PWM ||
            Connector::m_mode == Connector::CPM_MODE_A_PWM_B_PWM) {
        m_bDutyCnt = (static_cast<uint32_t>(duty) * m_stepsPerSampleMax +
                      (UINT8_MAX / 2)) / UINT8_MAX;
        UpdateBDuty();
        return true;
    }
    return false;
}

void MotorDriver::EnableTriggerPulse(uint16_t pulseCount, uint32_t time_ms,
                                     bool blockUntilDone) {
    // If not enabled, just return without doing anything.
    if (!EnableRequest()) {
        return;
    }

    __disable_irq();
    if (m_enableTriggerActive) {
        m_enableTriggerPulseCount += (pulseCount * 2);
    }
    else if (pulseCount) {
        m_enableTriggerPulseStartMs = TimingMgr.Milliseconds();
        m_enableTriggerPulseCount = (pulseCount * 2);
        m_enableTriggerActive = true;
        ToggleEnable();
    }
    m_enableTriggerPulseLenMs = time_ms;
    __enable_irq();

    if (blockUntilDone) {
        while (EnableTriggerPulseActive()) {
            continue;
        }
    }
}

void MotorDriver::EnableRequest(bool value) {
    bool wasDisabled = !(m_isEnabled || m_isEnabling);
    m_enableRequestedState = value;
    // Make sure to disable if we're in a fault state.
    // Otherwise, faithfully handle the request.
    value = !m_inFault && value;

    __disable_irq();
    m_enableTriggerActive = false;
    m_enableTriggerPulseCount = 0;

    if (wasDisabled && value) {
        // If we're enabling, set the enable delay.
        m_enableCounter = CPM_ENABLE_DELAY;
        // Set the enabling state based on the enable request.
        m_isEnabling = true;
    }
    else if (!value) {
        m_isEnabled = false;
    }
    __enable_irq();

    // Invert the logic if necessary.
    if (m_mode == Connector::CPM_MODE_STEP_AND_DIR) {
        if (!value) {
            MoveStopAbrupt();
        }
        if (m_polarityInversions.bit.enableInverted) {
            value = !value;
        }
    }

    // Update the shift register.
    ShiftReg.ShifterState(value, m_enableMask);
}

void MotorDriver::ToggleEnable() {
    // Update the shift register.
    ShiftReg.ShifterStateToggle(m_enableMask);
}

bool MotorDriver::PolarityInvertSDEnable(bool invert) {
    if (m_mode == Connector::CPM_MODE_STEP_AND_DIR) {
        m_polarityInversions.bit.enableInverted = invert ? 1 : 0;
        EnableRequest(m_enableRequestedState);
        return true;
    }
    else {
        return false;
    }
}

bool MotorDriver::PolarityInvertSDDirection(bool invert) {
    if (m_mode == Connector::CPM_MODE_STEP_AND_DIR) {
        m_polarityInversions.bit.directionInverted = invert ? 1 : 0;
        return true;
    }
    else {
        return false;
    }
}

bool MotorDriver::PolarityInvertSDHlfb(bool invert) {
    if (m_mode == Connector::CPM_MODE_STEP_AND_DIR) {
        m_polarityInversions.bit.hlfbInverted = invert ? 1 : 0;
        // Force the HLFB filtering to re-evaluate by setting TicksLeft
        m_filterTicksLeft = 1;
        return true;
    }
    else {
        return false;
    }
}

void MotorDriver::Initialize(ClearCorePins clearCorePin) {
    // Initialize the output values
    DATA_OUTPUT_STATE(m_aInfo->gpioPort, m_aDataMask, true);
    DATA_OUTPUT_STATE(m_bInfo->gpioPort, m_bDataMask, true);
    EnableRequest(false);

    // Initialize the HLFB
    DigitalIn::Initialize(clearCorePin);

    // Disable mux
    PIN_CONFIGURATION(m_aInfo->gpioPort, m_aInfo->gpioPin,  0);
    PIN_CONFIGURATION(m_bInfo->gpioPort, m_bInfo->gpioPin,  0);

    // Setup a and b as output
    DATA_DIRECTION_OUTPUT(m_aInfo->gpioPort, m_aDataMask);
    DATA_DIRECTION_OUTPUT(m_bInfo->gpioPort, m_bDataMask);

    // Configure the MotorInAState MUX for TCC, but don't enable it unless we go
    // into that mode
    PMUX_SELECTION(m_aInfo->gpioPort, m_aInfo->gpioPin, PER_TIMER_ALT);

    // Configure the MotorInBState MUX for TCC, but don't enable it unless we go
    // into that mode
    PMUX_SELECTION(m_bInfo->gpioPort, m_bInfo->gpioPin, PER_TIMER_ALT);

    Mode(CPM_MODE_A_DIRECT_B_DIRECT);

    // Connect PAD to External Interrupt device
    PMUX_SELECTION(m_inputPort, m_inputDataBit, PER_EXTINT);
    PMUX_ENABLE(m_inputPort, m_inputDataBit);

    // Pointer to our TC
    static Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;
    TcCount16 *tcCount = &tc_modules[m_hlfbTcNum]->COUNT16;
    // Turn off TC to allow setup
    tcCount->CTRLA.bit.ENABLE = 0;
    // Sync clocks
    SYNCBUSY_WAIT(tcCount, TC_SYNCBUSY_ENABLE);

    tcCount->CTRLA.bit.SWRST = 1;
    SYNCBUSY_WAIT(tcCount, TC_SYNCBUSY_SWRST);

    tcCount->CTRLA.bit.PRESCSYNC = TC_CTRLA_PRESCSYNC_GCLK_Val;
    // Capture operation on channel 1 and 2 triggered from Event System
    tcCount->CTRLA.bit.COPEN0 = 0;
    tcCount->CTRLA.bit.COPEN1 = 0;
    // Enables capture on channel 1 and 2 (vs compare)
    tcCount->CTRLA.bit.CAPTEN0 = 1;
    tcCount->CTRLA.bit.CAPTEN1 = 1;
    // The LUPD bit is not affected on overflow/underflow and re-trigger event
    tcCount->CTRLA.bit.ALOCK = 0;
    // Prescaler: GCLK_TC
    tcCount->CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV1_Val;
    // Clock On Demand, TC will not request the clock when stopped
    tcCount->CTRLA.bit.ONDEMAND = 1;
    // Run in Standby, the TC continues to run in standby
    tcCount->CTRLA.bit.RUNSTDBY = 1;
    // Set the counter to 16 bit mode
    tcCount->CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16_Val;

    // Select the count event action
    tcCount->EVCTRL.bit.EVACT = TC_EVCTRL_EVACT_PPW_Val;
    // Enable async input events to the TC
    tcCount->EVCTRL.bit.TCEI = 1;
    // Disable match/capture event
    tcCount->EVCTRL.bit.MCEO0 = 0;
    tcCount->EVCTRL.bit.MCEO1 = 0;
    // Disable overflow/underflow event
    tcCount->EVCTRL.bit.OVFEO = 0;
    tcCount->EVCTRL.bit.TCINV = 1;

    // Setup External Interrupt resource
    EIC->CTRLA.bit.ENABLE = 0;
    // Enable the corresponding output events
    EIC->EVCTRL.reg |= 1 << m_hlfbInfo->extInt;
    // Enable asynchronous edge detection operation
    EIC->ASYNCH.reg |= 1 << m_hlfbInfo->extInt;
    // See 28.8.10 in data sheet to see CONFIG register structure. 3-bit field
    // every 4 bits.
    EIC->CONFIG[m_hlfbInfo->extInt / 8].reg |=
        (EIC_CONFIG_SENSE0_HIGH_Val << ((m_hlfbInfo->extInt & 7) * 4));
    EIC->INTENCLR.bit.EXTINT = 1 << m_hlfbInfo->extInt;

    EIC->CTRLA.bit.ENABLE = 1;
    SYNCBUSY_WAIT(EIC, EIC_SYNCBUSY_ENABLE);

    // Clock the channel
    SET_CLOCK_SOURCE(EVSYS_GCLK_ID_0 + m_hlfbEvt, 6);

    EvsysChannel *theEvCh = &EVSYS->Channel[m_hlfbEvt];
    // Configure the event user mux to connect the channel to the event
    EVSYS->USER[EVSYS_ID_USER_TC0_EVU + m_hlfbTcNum].reg = m_hlfbEvt + 1;
    theEvCh->CHINTFLAG.bit.EVD = 1;
    while (theEvCh->CHSTATUS.reg & EVSYS_CHSTATUS_RDYUSR) {
        continue;
    }
    // Configure the event channel to use the ExtInt event, async
    theEvCh->CHANNEL.reg =
        EVSYS_CHANNEL_EVGEN(EVSYS_ID_GEN_EIC_EXTINT_0 + m_hlfbInfo->extInt) |
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS;
    while (theEvCh->CHSTATUS.reg & EVSYS_CHSTATUS_BUSYCH) {
        continue;
    }
    // Turn on timer
    tcCount->CTRLA.bit.ENABLE = 1;
    // Sync clocks
    SYNCBUSY_WAIT(tcCount, TC_SYNCBUSY_ENABLE);

    // Initialize the Motor Status Registers
    m_statusRegMotor.reg = 0;
    m_statusRegMotorRisen.reg = 0;
    m_statusRegMotorFallen.reg = 0;

    m_initialized = true;
}

bool MotorDriver::Mode(ConnectorModes newMode) {
    // Bail out if we are already in the requested mode
    if (newMode == m_mode) {
        return true;
    }

    uint8_t ccIndex;
    uint32_t syncBusyMask;
    Tcc *theTcc;

    switch (newMode) {
        case CPM_MODE_A_PWM_B_PWM:
            // Stop any active S&D command
            MoveStopAbrupt();
            // Determine which TCC->CC value maps to InputA
            ccIndex = m_aInfo->tccPadNum % TccCcNum(m_aInfo->tccNum);
            syncBusyMask = TCC_SYNCBUSY_CC(1UL << ccIndex);
            theTcc = (tcc_modules[m_aInfo->tccNum]);
            // Block the interrupt and make sure that the PWM values are cleared
            __disable_irq();
            SYNCBUSY_WAIT(theTcc, syncBusyMask);
            m_aDutyCnt = 0;
            UpdateADuty();
            // Determine which TCC->CC value maps to InputB
            ccIndex = m_bInfo->tccPadNum % TccCcNum(m_bInfo->tccNum);
            syncBusyMask = TCC_SYNCBUSY_CC(1UL << ccIndex);
            theTcc = (tcc_modules[m_bInfo->tccNum]);
            SYNCBUSY_WAIT(theTcc, syncBusyMask);
            m_bDutyCnt = 0;
            UpdateBDuty();
            // Enable peripheral on port/pin A and B to use PWM on both
            PMUX_ENABLE(m_aInfo->gpioPort, m_aInfo->gpioPin);
            PMUX_ENABLE(m_bInfo->gpioPort, m_bInfo->gpioPin);
            m_mode = newMode;
            __enable_irq();
            break;
        case CPM_MODE_A_DIRECT_B_PWM:
        case CPM_MODE_STEP_AND_DIR:
            // Stop any active S&D command
            MoveStopAbrupt();
            // Determine which TCC->CC value maps to InputB
            ccIndex = m_bInfo->tccPadNum % TccCcNum(m_bInfo->tccNum);
            syncBusyMask = TCC_SYNCBUSY_CC(1UL << ccIndex);
            theTcc = (tcc_modules[m_bInfo->tccNum]);
            // Block the interrupt and make sure that the PWM/step value is
            // cleared
            __disable_irq();
            SYNCBUSY_WAIT(theTcc, syncBusyMask);
            m_bDutyCnt = 0;
            UpdateBDuty();
            // Enable peripheral on port/pin B to use PWM on B only
            PMUX_DISABLE(m_aInfo->gpioPort, m_aInfo->gpioPin);
            PMUX_ENABLE(m_bInfo->gpioPort, m_bInfo->gpioPin);
            m_mode = newMode;
            __enable_irq();
            break;
        case CPM_MODE_A_DIRECT_B_DIRECT:
            // Stop any active S&D command
            MoveStopAbrupt();
            // Disable peripheral on both port/pin A and B
            PMUX_DISABLE(m_aInfo->gpioPort, m_aInfo->gpioPin);
            PMUX_DISABLE(m_bInfo->gpioPort, m_bInfo->gpioPin);
            m_mode = CPM_MODE_A_DIRECT_B_DIRECT;
            break;
        default:
            return false;
    }

    return true;
}

void MotorDriver::UpdateADuty() {
    *m_aTccBuffer = m_aDutyCnt;
}

void MotorDriver::UpdateBDuty() {
    *m_bTccBuffer = m_bDutyCnt;
}

void MotorDriver::RefreshSlow() {
    if (!m_initialized) {
        return;
    }
    uint32_t currentTimeMs = TimingMgr.Milliseconds();
    if (m_enableTriggerActive &&
            (currentTimeMs - m_enableTriggerPulseStartMs >=
             m_enableTriggerPulseLenMs)) {
        m_enableTriggerPulseStartMs = currentTimeMs;
        if (!--m_enableTriggerPulseCount) {
            m_enableTriggerActive = false;
        }
        else {
            ToggleEnable();
        }
    }
}

void MotorDriver::FaultState(bool isFaulted) {
    m_inFault = isFaulted;
    // Let EnableRequest handle the fault condition logic
    EnableRequest(m_enableRequestedState);
}

} // ClearCore namespace
