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
#include <stdlib.h>
#include <sam.h>
#include "atomic_utils.h"
#include "CcioBoardManager.h"
#include "Connector.h"
#include "InputManager.h"
#include "MotorManager.h"
#include "StatusManager.h"
#include "SysManager.h"
#include "SysTiming.h"
#include "SysUtils.h"

#define HLFB_CARRIER_LOSS_ERROR_LIMIT (0)
#define HLFB_CARRIER_LOSS_STATE_CHANGE_MS (4)
#define HLFB_CARRIER_LOSS_STATE_CHANGE_SAMPLES (MS_TO_SAMPLES * HLFB_CARRIER_LOSS_STATE_CHANGE_MS)

// TODO - name police, this is the amount of time we allow a move to execute
// without expecting an HLFB reading to be back yet
#define HARD_STOP_MOVE_MS (25)
#define HARD_STOP_MOVE_SAMPLES (MS_TO_SAMPLES * HARD_STOP_MOVE_MS)

namespace ClearCore {

extern MotorManager &MotorMgr;
extern SysManager SysMgr;
extern SysTiming &TimingMgr;
extern CcioBoardManager &CcioMgr;
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
      m_enableConnector(CLEARCORE_PIN_INVALID),
      m_inputAConnector(CLEARCORE_PIN_INVALID),
      m_inputBConnector(CLEARCORE_PIN_INVALID),
      m_hlfbTcNum(hlfbTc),
      m_hlfbEvt(hlfbEvt),
      m_hlfbMode(HLFB_MODE_STATIC),
      m_hlfbWidth{0, 0},
      m_hlfbPeriod{0, 0},
      m_hlfbNoPwmSampleCount(2),
      m_hlfbCarrierFrequency(HLFB_CARRIER_45_HZ),
      m_hlfbCarrierLossStateChange_ms(HLFB_CARRIER_LOSS_STATE_CHANGE_MS_45_HZ),
      m_hlfbLastCarrierDetectTime(UINT32_MAX),
      m_hlfbDuty(HLFB_DUTY_UNKNOWN),
      m_hlfbState(HLFB_UNKNOWN),
      m_lastHlfbInputValue(false),
      m_hlfbPwmReadingPending(false),
      m_hlfbStateChangeCounter(MS_TO_SAMPLES * HLFB_CARRIER_LOSS_STATE_CHANGE_MS_45_HZ),
      m_polarityInversions(0),
      m_enableRequestedState(false),
      m_enableTriggerActive(false),
      m_enableTriggerPulseStartMs(0),
      m_enableTriggerPulseCount(0),
      m_enableTriggerPulseLenMs(25),
      m_aDutyCnt(0),
      m_bDutyCnt(0),
      m_inFault(false),
      m_statusRegMotor(0),
      m_statusRegMotorRisen(0),
      m_statusRegMotorFallen(0),
      m_statusRegMotorLast(0),
      m_alertRegMotor(0),
      m_initialized(false),
      m_isEnabling(false),
      m_isEnabled(false),
      m_hlfbCarrierLost(false),
      m_enableCounter(0),
      m_brakeOutputPin(CLEARCORE_PIN_INVALID),
      m_limitSwitchNeg(CLEARCORE_PIN_INVALID),
      m_limitSwitchPos(CLEARCORE_PIN_INVALID),
      m_eStopConnector(CLEARCORE_PIN_INVALID),
      m_motionCancellingEStop(false),
      m_shiftRegEnableReq(false),
      m_clearFaultState(CLEAR_FAULT_IDLE),
      m_clearFaultHlfbTimer(0) {

    m_interruptAvail = true;

    Tcc *theTcc = (tcc_modules[m_aInfo->tccNum]);
    uint8_t ccIndex = m_aInfo->tccPadNum % TccCcNum(m_aInfo->tccNum);
    m_aTccBuffer = &theTcc->CCBUF[ccIndex].reg;
    m_aTccSyncMask = TCC_SYNCBUSY_CC(1UL << ccIndex);
    m_aTccSyncReg = &theTcc->SYNCBUSY.reg;

    theTcc = (tcc_modules[m_bInfo->tccNum]);
    ccIndex = m_bInfo->tccPadNum % TccCcNum(m_bInfo->tccNum);
    m_bTccBuffer = &theTcc->CCBUF[ccIndex].reg;
    m_bTccSyncMask = TCC_SYNCBUSY_CC(1UL << ccIndex);
    m_bTccSyncReg = &theTcc->SYNCBUSY.reg;
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
        case HLFB_MODE_HAS_PWM:
        case HLFB_MODE_HAS_BIPOLAR_PWM:
            // Check for overflow or error conditions
            if ((intFlagReg & (TC_INTFLAG_OVF | TC_INTFLAG_ERR)) ||
                (Milliseconds() - m_hlfbLastCarrierDetectTime
                    >= HLFB_CARRIER_LOSS_STATE_CHANGE_MS)) {
                tcCount->INTFLAG.reg = TC_INTFLAG_OVF | TC_INTFLAG_MC0 |
                                       TC_INTFLAG_ERR | TC_INTFLAG_MC1;
                // Saturating increment
                m_hlfbNoPwmSampleCount = __QADD16(m_hlfbNoPwmSampleCount, 1U);
                m_hlfbCarrierLost =
                    m_hlfbNoPwmSampleCount > HLFB_CARRIER_LOSS_ERROR_LIMIT;
            }
            // Did we capture a period?
            if (intFlagReg & TC_INTFLAG_MC0) {
                m_hlfbLastCarrierDetectTime = Milliseconds();

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
                m_hlfbStateChangeCounter = (MS_TO_SAMPLES * m_hlfbCarrierLossStateChange_ms);
                break;
            }
            else {
                // check for an HLFB state change
                bool readHlfbState = (DigitalIn::m_stateFiltered ^ invert);
                if (readHlfbState != m_lastHlfbInputValue) {
                    m_hlfbStateChangeCounter = (MS_TO_SAMPLES * m_hlfbCarrierLossStateChange_ms);
                    m_lastHlfbInputValue = readHlfbState;
                    break;
                }
                else if (m_hlfbStateChangeCounter && m_hlfbStateChangeCounter--) {
                    break;
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

    // Read associated input connectors and write associated output connectors.
    if (m_enableConnector != CLEARCORE_PIN_INVALID) {
        // Update the Enable state with the value on the Enable connector.
        Connector *input= SysMgr.ConnectorByIndex(m_enableConnector);
        if (input->Type() == ClearCore::Connector::CCIO_DIGITAL_IN_OUT_TYPE) {
            EnableRequest(CcioMgr.PinState(m_enableConnector));
        }
        else {
            DigitalIn *enableIn = static_cast<DigitalIn *>(input);
            EnableRequest(enableIn->DigitalIn::State());
        }
    }
    if (m_inputAConnector != CLEARCORE_PIN_INVALID && m_mode != CPM_MODE_STEP_AND_DIR) {
        // Update the Input A state with the value on the Input A connector.
        Connector *input= SysMgr.ConnectorByIndex(m_inputAConnector);
        if (input->Type() == ClearCore::Connector::CCIO_DIGITAL_IN_OUT_TYPE) {
            MotorInAState(CcioMgr.PinState(m_inputAConnector));
        }
        else {
            DigitalIn *inputA = static_cast<DigitalIn *>(input);
            MotorInAState(inputA->DigitalIn::State());
        }
    }
    if (m_inputBConnector != CLEARCORE_PIN_INVALID && m_mode != CPM_MODE_STEP_AND_DIR) {
        // Update the Input B state with the value on the Input B connector.
        Connector *input= SysMgr.ConnectorByIndex(m_inputBConnector);
        if (input->Type() == ClearCore::Connector::CCIO_DIGITAL_IN_OUT_TYPE) {
            MotorInBState(CcioMgr.PinState(m_inputBConnector));
        }
        else {
            DigitalIn *inputB = static_cast<DigitalIn *>(input);
            MotorInBState(inputB->DigitalIn::State());
        }
    }
    if (m_brakeOutputPin != CLEARCORE_PIN_INVALID) {
        Connector *brakeOutput = SysMgr.ConnectorByIndex(m_brakeOutputPin);
        if (brakeOutput->Type() == CCIO_DIGITAL_IN_OUT_TYPE ||
        brakeOutput->Mode() == ConnectorModes::OUTPUT_DIGITAL) {
            // Using HLFB_MODE_STATIC assumes the motor is in Servo On HLFB mode
            if (m_hlfbMode == HLFB_MODE_STATIC) {
                brakeOutput->State(static_cast<int16_t>(m_hlfbState == HLFB_ASSERTED));
            }
            else {
                brakeOutput->State(static_cast<int16_t>(m_hlfbState != HLFB_DEASSERTED));
            }
        }
    }
    if (m_limitSwitchPos != CLEARCORE_PIN_INVALID) {
        // Update the positive limit state with the value on the connector.
        Connector *input = SysMgr.ConnectorByIndex(m_limitSwitchPos);
        if (input->Type() == CCIO_DIGITAL_IN_OUT_TYPE) {
            PosLimitActive(!input->State());
        }
        else {
            DigitalIn *inputB = static_cast<DigitalIn *>(input);
            PosLimitActive(!inputB->DigitalIn::State());
        }
    }
    if (m_limitSwitchNeg != CLEARCORE_PIN_INVALID) {
        // Update the negative limit state with the value on the connector.
        Connector *input = SysMgr.ConnectorByIndex(m_limitSwitchNeg);
        if (input->Type() == CCIO_DIGITAL_IN_OUT_TYPE) {
            NegLimitActive(!input->State());
        }
        else {
            DigitalIn *inputB = static_cast<DigitalIn *>(input);
            NegLimitActive(!inputB->DigitalIn::State());
        }
    }


    // Update the Motor Status and Alert Registers
    StatusRegMotor statusRegPending = m_statusRegMotor;
    AlertRegMotor alertRegPending = m_alertRegMotor;

    // Check E-stop
    bool eStopInput = CheckEStopSensor();
    if (m_moveState == MS_IDLE) {
        m_motionCancellingEStop = false;
    }
    else if (eStopInput && !m_motionCancellingEStop) {
        MoveStopDecel();
        m_motionCancellingEStop = true;
        alertRegPending.bit.MotionCanceledSensorEStop = 1;
    }
    statusRegPending.bit.InEStopSensor = (eStopInput || m_motionCancellingEStop);

    // Check limits
    if (!m_lastMoveWasPositional && m_statusRegMotor.bit.StepsActive) {
        if (m_direction && m_limitInfo.InNegHWLimit) {
            alertRegPending.bit.MotionCanceledNegativeLimit = 1;
        }
        else if (!m_direction && m_limitInfo.InPosHWLimit) {
            alertRegPending.bit.MotionCanceledPositiveLimit = 1;
        }
    }

    statusRegPending.bit.InPositiveLimit = m_limitInfo.InPosHWLimit;
    statusRegPending.bit.InNegativeLimit = m_limitInfo.InNegHWLimit;

    statusRegPending.bit.Triggering = m_enableTriggerActive;
    statusRegPending.bit.MoveDirection = StepGenerator::m_direction;
    statusRegPending.bit.StepsActive =
        (StepGenerator::m_moveState != StepGenerator::MoveStates::MS_IDLE &&
            StepGenerator::m_moveState != StepGenerator::MoveStates::MS_END);
    statusRegPending.bit.AtTargetPosition = m_isEnabled && 
        m_lastMoveWasPositional && !statusRegPending.bit.StepsActive &&
        m_hlfbState == HLFB_ASSERTED;
    statusRegPending.bit.AtTargetVelocity = m_isEnabled &&
        (StepGenerator::m_moveState == StepGenerator::MoveStates::MS_CRUISE ||
        (!statusRegPending.bit.StepsActive && !m_lastMoveWasPositional)) &&
        m_hlfbState != HLFB_DEASSERTED;
    statusRegPending.bit.PositionalMove = m_lastMoveWasPositional;

    statusRegPending.bit.HlfbState = m_hlfbState;

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
        if (statusRegPending.bit.StepsActive) {
            alertRegPending.bit.MotionCanceledMotorDisabled = 1;
        }
    }
    else if (m_isEnabling) {
        statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_ENABLING;
    }
    else {
        if (m_hlfbMode != HLFB_MODE_STATIC &&
            m_hlfbState == HlfbStates::HLFB_DEASSERTED) {
            statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_FAULTED;
            statusRegPending.bit.MotorInFault = 1;
            alertRegPending.bit.MotorFaulted = 1;
            MoveStopAbrupt();

        }
        else if ((m_hlfbMode == HLFB_MODE_STATIC &&
                  m_hlfbState == MotorDriver::HlfbStates::HLFB_DEASSERTED) ||
                 statusRegPending.bit.StepsActive) {
            statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_MOVING;
        }
        else {
            statusRegPending.bit.ReadyState = MotorReadyStates::MOTOR_READY;
            statusRegPending.bit.MotorInFault = 0;
        }
    }

    if (statusRegPending.bit.StepsActive) {
        if (alertRegPending.bit.MotorFaulted) {
            alertRegPending.bit.MotionCanceledInAlert = 1;
        }
    }

    statusRegPending.bit.AlertsPresent = (bool)alertRegPending.reg;
    m_statusRegMotor = statusRegPending;
    m_alertRegMotor = alertRegPending;

    atomic_or_fetch(&m_statusRegMotorRisen.reg,
                    ~m_statusRegMotorLast.reg & statusRegPending.reg);
    atomic_or_fetch(&m_statusRegMotorFallen.reg,
                    m_statusRegMotorLast.reg & ~statusRegPending.reg);

    m_statusRegMotorLast.reg = m_statusRegMotor.reg;

    // Calculate the next S&D output step count
    if (Connector::m_mode == Connector::CPM_MODE_STEP_AND_DIR) {
        // Calculate the number of steps to send in the next sample time
        StepGenerator::StepsCalculated();
        // Check the status of the limits
        StepGenerator::CheckTravelLimits();

        m_bDutyCnt = StepGenerator::m_stepsPrevious;
        // Queue up the steps by writing the B duty value
        UpdateBDuty();
    }
}

bool MotorDriver::ValidateMove(bool negDirection) {
    bool valid = true;
    if (m_alertRegMotor.reg) {
        m_alertRegMotor.bit.MotionCanceledInAlert = 1;
        valid = false;
    }
    if (!EnableRequest()) {
        m_alertRegMotor.bit.MotionCanceledMotorDisabled = 1;
        valid = false;
    }
    if (CheckEStopSensor()) {
        m_alertRegMotor.bit.MotionCanceledSensorEStop = 1;
        valid = false;
    }
    // Check +/- hardware limits
    if (negDirection && m_limitInfo.InNegHWLimit) {
        m_alertRegMotor.bit.MotionCanceledNegativeLimit = 1;
        valid = false;
    }
    else if (!negDirection && m_limitInfo.InPosHWLimit) {
        m_alertRegMotor.bit.MotionCanceledPositiveLimit = 1;
        valid = false;
    }
    return valid;
}

bool MotorDriver::Move(int32_t dist, MoveTarget moveTarget) {
    bool negDir;

    if (moveTarget == MOVE_TARGET_ABSOLUTE) {
        negDir = dist - m_posnAbsolute < 0;
    }
    else {
        negDir = dist < 0;
    }

    if (!ValidateMove(negDir)) {
        if (m_statusRegMotor.bit.StepsActive ) {
            MoveStopDecel();
        }
        return false;
    }

    m_lastMoveWasPositional = true;
    return StepGenerator::Move(dist, moveTarget);
}

bool MotorDriver::MoveVelocity(int32_t velocity) {
    if (!ValidateMove(velocity < 0)) {
        if (m_statusRegMotor.bit.StepsActive ) {
            MoveStopDecel();
        }
        return false;
    }
    m_lastMoveWasPositional = false;
    return StepGenerator::MoveVelocity(velocity);
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
    switch (m_mode) {
        case Connector::CPM_MODE_A_DIRECT_B_DIRECT:
        case Connector::CPM_MODE_A_DIRECT_B_PWM:
            DATA_OUTPUT_STATE(m_aInfo->gpioPort, m_aDataMask, !value);
            return true;
        case Connector::CPM_MODE_A_PWM_B_PWM:
            //MotorInACount(value ? m_stepsPerSampleMax : 0);
            //return true;
        case Connector::CPM_MODE_STEP_AND_DIR:
        default:
            return false;
    }
}

bool MotorDriver::MotorInBState(bool value) {
    switch (m_mode) {
        case Connector::CPM_MODE_A_DIRECT_B_DIRECT:
            DATA_OUTPUT_STATE(m_bInfo->gpioPort, m_bDataMask, !value);
            return true;
        case Connector::CPM_MODE_A_DIRECT_B_PWM:
        case Connector::CPM_MODE_A_PWM_B_PWM:
            //MotorInBCount(value ? m_stepsPerSampleMax : 0);
            //return true;
        case Connector::CPM_MODE_STEP_AND_DIR:
        default:
            return false;
    }
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

bool MotorDriver::MotorInACount(uint16_t count) {
    if (Connector::m_mode == Connector::CPM_MODE_A_PWM_B_PWM) {
        m_aDutyCnt = count;
        UpdateADuty();
        return true;
    }
    return false;
}

bool MotorDriver::MotorInBCount(uint16_t count) {
    if (Connector::m_mode == Connector::CPM_MODE_A_DIRECT_B_PWM ||
    Connector::m_mode == Connector::CPM_MODE_A_PWM_B_PWM) {
        m_bDutyCnt = count;
        UpdateBDuty();
        return true;
    }
    return false;
}

void MotorDriver::EnableTriggerPulse(uint16_t pulseCount, uint32_t time_ms,
                                     bool blockUntilDone) {
    // If not enabled, just return without doing anything.
    if (!EnableRequest() || m_inFault) {
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
    bool wasPulsing = m_enableTriggerActive;

    if (value != m_enableRequestedState || m_inFault) {
        m_enableTriggerActive = false;
        m_enableTriggerPulseCount = 0;
    }

    m_enableRequestedState = value;
    // Make sure to disable if we're in a fault state.
    // Otherwise, faithfully handle the request.
    value = !m_inFault && value;

    __disable_irq();

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
        if (!value && m_statusRegMotor.bit.StepsActive) {
            m_alertRegMotor.bit.MotionCanceledMotorDisabled = 1;
            MoveStopAbrupt();
        }
        if (m_polarityInversions.bit.enableInverted) {
            value = !value;
        }
    }

    if (value != m_shiftRegEnableReq || (wasPulsing && !m_enableTriggerActive)) {
        // Update the shift register.
        ShiftReg.ShifterState(value, m_enableMask);
    }
     m_shiftRegEnableReq = value;
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

bool MotorDriver::BrakeOutput(ClearCorePins pin) {
    if (pin != m_brakeOutputPin && m_brakeOutputPin != CLEARCORE_PIN_INVALID) {
        // Reset the state of the previously-set brake output connector
        SysMgr.ConnectorByIndex(m_brakeOutputPin)->State(false);
    }

    return SetConnector(pin, m_brakeOutputPin, false);
}

bool MotorDriver::LimitSwitchPos(ClearCorePins pin) {
    bool retVal = SetConnector(pin, m_limitSwitchPos);
    if (m_limitSwitchPos == CLEARCORE_PIN_INVALID) {
        PosLimitActive(false);
    }
    return retVal;
}

bool MotorDriver::LimitSwitchNeg(ClearCorePins pin) {
    bool retVal = SetConnector(pin, m_limitSwitchNeg);
    if (m_limitSwitchNeg == CLEARCORE_PIN_INVALID) {
        NegLimitActive(false);
    }
    return retVal;
}

bool MotorDriver::EnableConnector(ClearCorePins pin) {
    return SetConnector(pin, m_enableConnector);
}

bool MotorDriver::InputAConnector(ClearCorePins pin) {
    return SetConnector(pin, m_inputAConnector);
}

bool MotorDriver::InputBConnector(ClearCorePins pin) {
    return SetConnector(pin, m_inputBConnector);
}

bool MotorDriver::EStopConnector(ClearCorePins pin) {
    return SetConnector(pin, m_eStopConnector);
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

    switch (newMode) {
        case CPM_MODE_A_PWM_B_PWM:
            // Stop any active S&D command
            MoveStopAbrupt();
            // Block the interrupt and make sure that the PWM values are cleared
            __disable_irq();
            m_aDutyCnt = 0;
            UpdateADuty();
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
            // Block the interrupt and make sure that the PWM/step value is
            // cleared
            __disable_irq();
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
    if (*m_aTccBuffer == m_aDutyCnt) {
        return;
    }
    while (*m_aTccSyncReg & m_aTccSyncMask) {
        continue;
    }
    *m_aTccBuffer = m_aDutyCnt;
}

void MotorDriver::UpdateBDuty() {
    if (*m_bTccBuffer == m_bDutyCnt) {
        return;
    }
    while (*m_bTccSyncReg & m_bTccSyncMask) {
        continue;
    }
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

    switch (m_clearFaultState) {
        case CLEAR_FAULT_PULSE_ENABLE:
            if (m_enableTriggerActive) {
                break;
            }
            m_clearFaultState = CLEAR_FAULT_WAIT_FOR_HLFB;
            // Fall through
        case CLEAR_FAULT_WAIT_FOR_HLFB:
            if (m_hlfbState != HLFB_DEASSERTED) {
                AlertRegMotor mask;
                mask.bit.MotorFaulted = 1;
                ClearAlerts(mask.reg);
                m_clearFaultState = CLEAR_FAULT_IDLE;
            }
            else if (!(m_clearFaultHlfbTimer && m_clearFaultHlfbTimer--)) {
                m_clearFaultState = CLEAR_FAULT_IDLE;
            }
            break;
        case CLEAR_FAULT_IDLE:
        default:
            break;
    }
}

void MotorDriver::FaultState(bool isFaulted) {
    m_inFault = isFaulted;
    // Let EnableRequest handle the fault condition logic
    EnableRequest(m_enableRequestedState);
}

bool MotorDriver::SetConnector(ClearCorePins pin, ClearCorePins &memberPin,
                               bool input) {
    if (pin == memberPin) {
        // Nothing to do; already assigned.
        return true;
    }

    // Validate the pin type. If it checks out, write the pin value into the
    // memberPin member variable. Allow setting an "invalid" pin value to
    // disable the associated feature.
    if ((pin == CLEARCORE_PIN_INVALID) || 
        (input && IsValidInputPin(pin)) ||
        (!input && IsValidOutputPin(pin))) {
        memberPin = pin;
        return true;
    }

    return false; // Pin supplied was invalid.
}

bool MotorDriver::IsValidOutputPin(ClearCorePins pin) {
    // Pins IO-0 through IO-5 and all CCIO-8 connectors are the only
    // valid digital output pins available.
    return (pin >= CLEARCORE_PIN_IO0 && pin <= CLEARCORE_PIN_IO5) ||
           (pin >= CLEARCORE_PIN_CCIOA0 && pin <= CLEARCORE_PIN_CCIOH7);
}

bool MotorDriver::IsValidInputPin(ClearCorePins pin) {
    // Pins IO-0 through A-12 and all CCIO-8 connectors are the only
    // valid digital input pins available.
    return (pin >= CLEARCORE_PIN_IO0 && pin <= CLEARCORE_PIN_A12) ||
           (pin >= CLEARCORE_PIN_CCIOA0 && pin <= CLEARCORE_PIN_CCIOH7);
}

bool MotorDriver::CheckEStopSensor() {
    bool eStop = false;
    if (m_eStopConnector != CLEARCORE_PIN_INVALID) {
        Connector *input = SysMgr.ConnectorByIndex(m_eStopConnector);
        if (input->Type() == ClearCore::Connector::CCIO_DIGITAL_IN_OUT_TYPE) {
            eStop = !(input->State());
        }
        else {
            DigitalIn *inputB = static_cast<DigitalIn *>(input);
            eStop = !(inputB->DigitalIn::State());
        }
    }
    return eStop;
}

} // ClearCore namespace
