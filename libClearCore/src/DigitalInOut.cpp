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
    ClearCore Connector Digital Input/Output Class

    This manages an input/output connector on the ClearCore board.

    This class enhances an input point to include an output capability.
**/

#include "DigitalInOut.h"
#include <sam.h>
#include "StatusManager.h"
#include "SysTiming.h"
#include "SysUtils.h"

#define OVERLOAD_TRIP_TICKS ((uint8_t)(2.4 * MS_TO_SAMPLES))
#define OVERLOAD_FOLDBACK_TICKS (100 * MS_TO_SAMPLES)

namespace ClearCore {

extern StatusManager &StatusMgr;
extern ShiftRegister ShiftReg;
extern volatile uint32_t tickCnt;

/**
    Construct and wire in the Input/Output pair.
**/
DigitalInOut::DigitalInOut(ShiftRegister::Masks ledMask,
                           const PeripheralRoute *inputInfo,
                           const PeripheralRoute *outputInfo,
                           bool logicInversion)
    : DigitalIn(ledMask, inputInfo),
      m_outputPort(outputInfo->gpioPort),
      m_outputDataBit(outputInfo->gpioPin),
      m_outputDataMask(1UL << outputInfo->gpioPin),
      m_logicInversion(logicInversion),
      m_isInFault(false),
      m_tcPadNum(outputInfo->tcPadNum),
      m_outState(false),
      m_pulseOnTicks(0),
      m_pulseOffTicks(0),
      m_pulseStart(0),
      m_pulseStopCount(0),
      m_pulseCounter(0),
      m_overloadTripCnt(OVERLOAD_TRIP_TICKS),
      m_pulseActive(false),
      m_pulseValue(false),
      m_pulseStopPending(false),
      m_overloadFoldbackCnt(0) {
    static Tc *const tc_modules[TC_INST_NUM] = TC_INSTS;
    m_tc = tc_modules[outputInfo->tcNum];
}

bool DigitalInOut::Mode(ConnectorModes newMode) {
    // Bail out if we are already in the requested mode
    if (newMode == m_mode) {
        return true;
    }

    switch (newMode) {
        // Set up as output
        case OUTPUT_DIGITAL:
            m_overloadTripCnt = OVERLOAD_TRIP_TICKS;
            m_overloadFoldbackCnt = 0;
            m_mode = newMode;
            State(m_outState);
            ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);
            PMUX_DISABLE(m_outputPort, m_outputDataBit);
            break;
        // Set up as input
        case INPUT_DIGITAL:
            m_mode = newMode;
            // Force set output to avoid fault condition
            m_pulseActive = false;
            m_pulseStopPending = false;
            OutputPin(false);
            ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);
            PMUX_DISABLE(m_outputPort, m_outputDataBit);
            // Fault state only applies in output digital mode
            IsInHwFault(false);
            break;
        case OUTPUT_PWM:
            m_mode = newMode;
            State(0);
            ShiftReg.LedInPwm(m_ledMask, true, m_clearCorePin);
            PMUX_ENABLE(m_outputPort, m_outputDataBit);
            // Fault state only applies in output digital mode
            IsInHwFault(false);
            break;
        // Unsupported mode, don't change anything
        default:
            break;
    }

    return (m_mode == newMode);
}

/**
    Update connector's state.
**/
void DigitalInOut::Refresh() {
    DigitalIn::Refresh();

    switch (m_mode) {
        case OUTPUT_DIGITAL:
            if (m_overloadFoldbackCnt) {
                if (!(--m_overloadFoldbackCnt)) {
                    // Coming out of foldback, reset the overload
                    // delay timer and restore the pin state
                    OutputPin(m_outState);
                    m_overloadTripCnt = OVERLOAD_TRIP_TICKS;
                }
            }
            // If output is true and input is false, the pin is overloaded
            else if (m_outState && !StateRT()) {
                // When the overload counter hits zero, signal the overload
                if (m_overloadTripCnt && !--m_overloadTripCnt) {
                    IsInHwFault(true);
                    OutputPin(false);
                    m_overloadFoldbackCnt = OVERLOAD_FOLDBACK_TICKS;
                }
            }
            else {
                // Not overloaded, reset the overload delay timer
                m_overloadTripCnt = OVERLOAD_TRIP_TICKS;
                IsInHwFault(false);
            }
            if (!m_pulseActive) {
                break;
            }

            if (m_pulseStopCount == 0 || m_pulseCounter < m_pulseStopCount) {
                if (m_pulseValue) {
                    if (tickCnt - m_pulseStart >= m_pulseOnTicks) {
                        // Turn off the pulse
                        m_pulseValue = false;
                        m_pulseStart = tickCnt;
                        // Reset the filter when the output changes (to
                        // prevent an overload condition being falsely reported)
                        m_overloadTripCnt = OVERLOAD_TRIP_TICKS;
                        OutputPin(false);
                        m_outState = false;
                        // Increment the counter after a complete cycle
                        ++m_pulseCounter;
                        // If a stop is pending, handle it now that the cycle
                        // has completed
                        if (m_pulseStopPending) {
                            m_pulseActive = false;
                            m_pulseStopPending = false;
                        }
                    }
                }
                else {
                    // If a stop is pending, handle it right away while the
                    // pulseValue is low
                    if (m_pulseStopPending) {
                        m_pulseActive = false;
                        m_pulseStopPending = false;
                    }
                    else if (tickCnt - m_pulseStart >= m_pulseOffTicks) {
                        // Turn on the pulse
                        m_pulseValue = true;
                        m_pulseStart = tickCnt;
                        // Reset the filter when the output changes (to
                        // prevent an overload condition being falsely reported)
                        m_overloadTripCnt = OVERLOAD_TRIP_TICKS;
                        // Assert the output pin if we are not in overload
                        // foldback
                        OutputPin(!m_overloadFoldbackCnt);
                        m_outState = true;
                    }
                }
            }
            else if (m_pulseCounter == m_pulseStopCount) {
                m_pulseActive = false;
                m_pulseStopPending = false;
            }
            break;
        case OUTPUT_PWM:
        case INPUT_DIGITAL:
        default:
            break;
    }
}

int16_t DigitalInOut::State() {
    int16_t state = -1;

    switch (m_mode) {
        case OUTPUT_DIGITAL:
            state = m_outState;
            break;
        case INPUT_DIGITAL:
            // Return internal state, preventing re-read
            state = DigitalIn::State();
            break;
        case OUTPUT_PWM:
            if (!m_tc) {
                break;
            }

            if (m_logicInversion) {
                state = m_tc->COUNT8.CCBUF[m_tcPadNum].reg;
            }
            else {
                state = 255 - m_tc->COUNT8.CCBUF[m_tcPadNum].reg;
            }
            break;
        default:
            state = 0;
    }

    return state;
}

bool DigitalInOut::State(int16_t newState) {
    bool success = false;
    m_pulseActive = false;
    m_pulseStopPending = false;

    switch (m_mode) {
        case OUTPUT_DIGITAL:
            if (m_outState != static_cast<bool>(newState)) {
                m_overloadTripCnt = OVERLOAD_TRIP_TICKS;
                m_outState = static_cast<bool>(newState);
            }
            OutputPin(newState && !m_overloadFoldbackCnt);
            success = true;
            break;
        case INPUT_DIGITAL:
            // Not writable in input mode
            success = false;
            break;
        case OUTPUT_PWM:
            // Cap the input at max PWM (255)
            if (static_cast<uint16_t>(newState) > UINT8_MAX) {
                newState = UINT8_MAX;
            }
            // Start the PWM output
            success = PwmDuty(newState);
            break;
        default:
            break;
    }

    return success;
}

/**
    Initialize a digital input/output connector. Set to input mode.
**/
void DigitalInOut::Initialize(ClearCorePins clearCorePin) {
    // Clean up any state that Reinitialize may require
    m_outState = false;
    m_isInFault = false;
    m_pulseActive = false;
    m_pulseStopPending = false;

    // Set up to multiplex with TC for periodic user output functions
    PMUX_SELECTION(m_outputPort, m_outputDataBit, PER_TIMER);

    // Perform any input initializing
    DigitalIn::Initialize(clearCorePin);
    // Mode is now set to INPUT_DIGITAL

    // Set up output pin as output now that the connector is initialized
    DATA_DIRECTION_OUTPUT(m_outputPort, m_outputDataMask);
}

void DigitalInOut::OutputPulsesStart(uint32_t onTime, uint32_t offTime,
                                     uint16_t pulseCount, bool blockUntilDone) {
    // Do not start output pulses if we are in input mode
    if (!IsWritable()) {
        return;
    }
    // Ignore pulses that never turn on or off
    if (onTime == 0 || offTime == 0) {
        return;
    }

    Mode(OUTPUT_DIGITAL);
    m_pulseOnTicks = onTime * MS_TO_SAMPLES;
    m_pulseOffTicks = offTime * MS_TO_SAMPLES;

    m_pulseStopPending = false;
    m_pulseCounter = 0;
    m_pulseStopCount = pulseCount;

    if (!OutputPulsesActive()) {
        m_pulseStart = tickCnt;
        m_pulseActive = true;
        m_pulseValue = true;
        // Turn on output directly (calling State() will stop the pulse)
        m_overloadTripCnt = OVERLOAD_TRIP_TICKS;
        OutputPin(!m_overloadFoldbackCnt);
        m_outState = true;
    }

    if (blockUntilDone && pulseCount != 0) {
        while (OutputPulsesActive()) {
            continue;
        }
    }
}

void DigitalInOut::OutputPulsesStop(bool stopImmediately) {
    // Always set output to low after clearing a pulse
    if (stopImmediately) {
        // State will automatically clear m_pulseActive
        State(false);
    }
    else {
        m_pulseStopPending = true;
    }
}

bool DigitalInOut::PwmDuty(uint8_t newDuty) {
    // Bail out if not in PWM output mode
    if (m_mode != OUTPUT_PWM) {
        return false;
    }

    // Wait for the TC CC value to be ready to be written
    uint8_t ccBufVal = (m_logicInversion) ? newDuty : 255 - newDuty;
    if (m_tc->COUNT8.CCBUF[m_tcPadNum].reg != ccBufVal) {
        uint32_t syncMask = m_tcPadNum ? TC_SYNCBUSY_CC1 : TC_SYNCBUSY_CC0;
        SYNCBUSY_WAIT(&m_tc->COUNT8, syncMask);
        m_tc->COUNT8.CCBUF[m_tcPadNum].reg = ccBufVal;
    }
    ShiftReg.LedPwmValue(m_clearCorePin, newDuty);
    return true;
}

void DigitalInOut::IsInHwFault(bool inFault) {
    if (inFault != m_isInFault) {
        m_isInFault = inFault;
        ShiftReg.LedInFault(m_ledMask, m_isInFault);
        StatusMgr.OverloadUpdate(1UL << m_clearCorePin, m_isInFault);
        if (inFault) {
            StatusMgr.BlinkCode(
                BlinkCodeDriver::BLINK_GROUP_IO_OVERLOAD,
                1UL << m_clearCorePin);
        }
    }
}

} // ClearCore namespace
