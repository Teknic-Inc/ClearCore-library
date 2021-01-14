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
    DigitalInOutHBridge implementation

    Implements an H-Bridge, PWM output, and/or a digital input.
**/

#include "DigitalInOutHBridge.h"
#include <arm_math.h>
#include <sam.h>
#include <stdlib.h>
#include "DigitalInOut.h"
#include "StatusManager.h"
#include "SysTiming.h"
#include "SysUtils.h"

extern uint32_t SystemCoreClock;

namespace ClearCore {

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

// PWM related constants
#define TONE_RATE_HZ (22050)
#define TONE_MAXIMUM_FREQ_HZ (TONE_RATE_HZ / 4)

extern ShiftRegister ShiftReg;
extern volatile uint32_t tickCnt;

DigitalInOutHBridge::DigitalInOutHBridge(ShiftRegister::Masks ledMask,
        const PeripheralRoute *inputInfo,
        const PeripheralRoute *outputInfo,
        const PeripheralRoute *pwmAInfo,
        const PeripheralRoute *pwmBInfo,
        IRQn_Type tccIrq,
        bool invertDigitalLogic)
    : DigitalInOut(ledMask,
                   inputInfo,
                   outputInfo,
                   invertDigitalLogic),
      m_amplitude(INT16_MAX / 10),
      m_sinStep(0),
      m_angle(0),
      m_toneStartTick(0),
      m_toneOnTicks(0),
      m_toneOffTicks(0),
      m_toneState(TONE_OFF),
      m_pwmAInfo(pwmAInfo),
      m_pwmBInfo(pwmBInfo),
      m_tccIrq(tccIrq),
      m_inFault(false),
      m_forceToneDuration(false) {
    static Tcc *const tcc_modules[TCC_INST_NUM] = TCC_INSTS;
    m_tcc = tcc_modules[pwmAInfo->tccNum];
}

bool DigitalInOutHBridge::IsWritable() {
    bool isWritable;

    switch (m_mode) {
        case OUTPUT_DIGITAL:
        case OUTPUT_PWM:
        case OUTPUT_H_BRIDGE:
        case OUTPUT_TONE:
        case OUTPUT_WAVE:
            isWritable = true;
            break;
        case INPUT_DIGITAL:
        default:
            isWritable = false;
            break;
    }
    return isWritable;
}

int16_t DigitalInOutHBridge::State() {
    int16_t state;

    switch (m_mode) {
        case INPUT_DIGITAL:
        case OUTPUT_DIGITAL:
        case OUTPUT_PWM:
            // Let the DigitalInOut handle it.
            state = DigitalInOut::State();
            break;
        case OUTPUT_H_BRIDGE:
        case OUTPUT_TONE:
            // Undo the state math to return the current state value
            state =
                static_cast<int16_t>((static_cast<int32_t>(m_tcc->CC[0].reg) -
                                      (m_tcc->PER.reg >> 1)) * INT16_MAX / m_tcc->PER.reg >> 1);
            break;
        default:
            state = 0;
            break;
    }
    return state;
}

bool DigitalInOutHBridge::State(int16_t newState) {
    bool success = false;

    uint16_t halfDuty = m_tcc->PER.reg >> 1; // 50% duty cycle

    switch (m_mode) {
        case INPUT_DIGITAL:
        case OUTPUT_DIGITAL:
        case OUTPUT_PWM:
            // Let the DigitalInOut handle it.
            success = DigitalInOut::State(newState);
            break;
        case OUTPUT_H_BRIDGE:
            if (newState == INT16_MIN) {
                ShiftReg.LedPwmValue(m_clearCorePin, UINT8_MAX);
            }
            else {
                ShiftReg.LedPwmValue(m_clearCorePin, labs(newState) >> 7);
            }
        // Fall through
        case OUTPUT_TONE:
            // Create a PWM differential where state 0 is 50/50 duty cycles
            m_tcc->CCBUF[0].reg = halfDuty + halfDuty * newState / INT16_MAX;
            m_tcc->CCBUF[1].reg = halfDuty - halfDuty * newState / INT16_MAX;
            success = true;
            break;
        default:
            break;
    }

    return success;
}

void DigitalInOutHBridge::Refresh() {
    switch (m_mode) {
        case INPUT_DIGITAL:
        case OUTPUT_DIGITAL:
        case OUTPUT_PWM:
            // Let the DigitalInOut handle it.
            DigitalInOut::Refresh();
            break;
        case OUTPUT_WAVE:
            break;
        case OUTPUT_H_BRIDGE:
            break;
        case OUTPUT_TONE:
            switch (m_toneState) {
                case TONE_CONTINUOUS:
                    break;
                case TONE_TIMED:
                    if (tickCnt - m_toneStartTick > m_toneOnTicks) {
                        m_toneState = TONE_OFF;
                        m_forceToneDuration = false;
                        ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);
                    }
                    break;
                case TONE_PERIODIC_ON:
                    if (tickCnt - m_toneStartTick > m_toneOnTicks) {
                        m_toneStartTick = tickCnt;
                        m_toneState = TONE_PERIODIC_OFF;
                        ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);
                    }
                    break;
                case TONE_PERIODIC_OFF:
                    if (tickCnt - m_toneStartTick > m_toneOffTicks) {
                        m_toneState = TONE_PERIODIC_ON;
                        // Interrupt every period
                        m_tcc->INTENSET.bit.OVF = 1;
                        m_toneStartTick = tickCnt;
                        ShiftReg.LedInPwm(m_ledMask, true, m_clearCorePin);
                    }
                    break;
                case TONE_OFF:
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void DigitalInOutHBridge::ToneUpdate() {
    int32_t nextAngle = static_cast<int32_t>(m_angle) + m_sinStep;
    // When a tone is active, or a tone is ending and we haven't yet reached
    // the end of the final sine wave, find the next value to output
    if ((ToneActiveState() && (ToneActiveState() != TONE_PERIODIC_OFF)) ||
            (m_mode == OUTPUT_TONE && nextAngle < INT16_MAX)) {
        // Increment the current angle and clip to q15 [0 +1)
        m_angle = nextAngle & INT16_MAX;
        State(static_cast<int16_t>((static_cast<int32_t>(arm_sin_q15(m_angle)) *
                                    m_amplitude) >> 15));
    }
    else {
        // End the tone output and disable the interrupt
        m_tcc->INTENCLR.bit.OVF = 1;
        m_angle = 0;
        State(0);
    }
}

void DigitalInOutHBridge::Initialize(ClearCorePins clearCorePin) {
    DigitalInOut::Initialize(clearCorePin);
    // Mode is now INPUT_DIGITAL
    m_inFault = false;

    // Set up the TCC used by this IO for PWM generation
    // Disable TCC (must be disabled to write enable-protected registers)
    m_tcc->CTRLA.bit.SWRST = 1;
    SYNCBUSY_WAIT(m_tcc, TCC_SYNCBUSY_SWRST);

    m_tcc->COUNT.reg = 0;
    // Allow the PWM to continue when breakpoints are hit
    m_tcc->DBGCTRL.reg = TCC_DBGCTRL_DBGRUN;
    // Enable double buffering
    m_tcc->CTRLBCLR.bit.LUPD = 1;
    // TCC using dual-slope bottom PWM
    m_tcc->WAVE.reg |= TCC_WAVE_WAVEGEN_DSBOTTOM;
    // Interrupt every period when in TONE_OUTPUT, off for now
    m_tcc->INTENCLR.bit.OVF = 1;
    // Set the period for the TCC
    m_tcc->PER.reg = SystemCoreClock / (TONE_RATE_HZ << 1) - 1;

    // For default value drive both channels low
    for (int8_t iChannel = 0; iChannel < 2; iChannel++) {
        m_tcc->CC[iChannel].reg = m_tcc->PER.reg;
    }

    // All of the non-input pins can be configured for output
    // The PMUX will just disconnect the CPU pin when the line is connected
    // to the TCC.
    DATA_OUTPUT_STATE(m_pwmAInfo->gpioPort, 1UL << m_pwmAInfo->gpioPin, false);
    DATA_OUTPUT_STATE(m_pwmBInfo->gpioPort, 1UL << m_pwmBInfo->gpioPin, true);
    DATA_DIRECTION_OUTPUT(m_pwmAInfo->gpioPort, 1UL << m_pwmAInfo->gpioPin);
    DATA_DIRECTION_OUTPUT(m_pwmBInfo->gpioPort, 1UL << m_pwmBInfo->gpioPin);

    // Configure the polarity pins to be driven by PWM
    // The Port PMUX has it's multiplexing divided into odd and even bits
    PMUX_SELECTION(m_pwmAInfo->gpioPort, m_pwmAInfo->gpioPin, PER_TIMER_ALT);
    PMUX_SELECTION(m_pwmBInfo->gpioPort, m_pwmBInfo->gpioPin, PER_TIMER_ALT);
}

void DigitalInOutHBridge::ToneFrequency(uint16_t frequency) {
    // Enforce a maximum frequency to stay under the Nyquist frequency
    if (frequency > TONE_MAXIMUM_FREQ_HZ) {
        frequency = TONE_MAXIMUM_FREQ_HZ;
    }
    m_sinStep = static_cast<int32_t>(INT16_MAX) * frequency / TONE_RATE_HZ;
}

void DigitalInOutHBridge::ToneAmplitude(int16_t amplitude) {
    m_amplitude = max(amplitude, 0);
    if (m_mode == OUTPUT_TONE) {
        ShiftReg.LedPwmValue(m_clearCorePin, m_amplitude >> 7);
    }
}

void DigitalInOutHBridge::ToneContinuous(uint16_t frequency) {
    if (m_mode != OUTPUT_TONE || m_forceToneDuration) {
        // A non-blocking timed tone is currently playing that must finish first
        return;
    }
    ShiftReg.LedInPwm(m_ledMask, true, m_clearCorePin);
    ToneFrequency(frequency);
    m_toneState = TONE_CONTINUOUS;
    // Interrupt every period
    m_tcc->INTENSET.bit.OVF = 1;
}

void DigitalInOutHBridge::ToneTimed(uint16_t frequency, uint32_t duration,
                                    bool blocking, bool forceDuration) {
    if (m_mode != OUTPUT_TONE) {
        return;
    }

    if (m_toneState == TONE_TIMED && m_forceToneDuration) {
        // There is already a timed tone with strict duration playing that
        // hasn't sounded for the full duration. Wait for it to finish
        // before generating a new tone.
        return;
    }

    ShiftReg.LedInPwm(m_ledMask, true, m_clearCorePin);
    ToneFrequency(frequency);
    m_toneStartTick = tickCnt;
    m_toneOnTicks = duration * MS_TO_SAMPLES;
    m_tcc->INTENSET.bit.OVF = 1; // Interrupt every period
    if (duration == 0) {
        m_toneState = TONE_CONTINUOUS;
    }
    else {
        m_toneState = TONE_TIMED;
        m_forceToneDuration = forceDuration;

        if (blocking) {
            while (ToneActiveState()) {
                continue;
            }
        }
    }
}

void DigitalInOutHBridge::TonePeriodic(uint16_t frequency, uint32_t timeOn,
                                       uint32_t timeOff) {
    if (m_mode != OUTPUT_TONE || m_forceToneDuration) {
        // A non-blocking timed tone is currently playing that must finish first
        return;
    }
    ShiftReg.LedInPwm(m_ledMask, true, m_clearCorePin);
    ToneFrequency(frequency);
    m_toneStartTick = tickCnt;
    m_toneOnTicks = timeOn * MS_TO_SAMPLES;
    m_toneOffTicks = timeOff * MS_TO_SAMPLES;
    m_toneState = TONE_PERIODIC_ON;
    // Interrupt every period
    m_tcc->INTENSET.bit.OVF = 1;
}

void DigitalInOutHBridge::ToneStop() {
    if (m_mode != OUTPUT_TONE) {
        // Wrong mode
        return;
    }
    if (!m_forceToneDuration) {
        m_toneState = TONE_OFF;
        ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);
    }
}

bool DigitalInOutHBridge::Mode(ConnectorModes newMode) {
    bool modeChangeSuccess = false;

    if (m_mode == newMode) {
        return true;
    }

    // Unless in H-Bridge, pwmA should be low and pwmB should be high
    // This makes the connector look like the other IO connectors.
    // In HBridge mode, these will be driven by the TCC
    // Note: having the TCC control a pin will disconnect the CPU pins.
    bool tccControlPwm = false;

    switch (newMode) {
        case INPUT_DIGITAL:
        case OUTPUT_DIGITAL:
        // Fall through. INPUT_DIGITAL, OUTPUT_DIGITAL, and OUTPUT_PWM
        // all are run by the DigitalInOut class
        case OUTPUT_PWM:
            // Update the LED pattern
            ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);
            modeChangeSuccess = DigitalInOut::Mode(newMode);
            break;
        case OUTPUT_TONE:
            ShiftReg.LedPwmValue(m_clearCorePin, m_amplitude >> 7);
        // H-BRIDGE, TONE, and WAVE have the same hardware
        // fall through
        case OUTPUT_H_BRIDGE:
        case OUTPUT_WAVE:
            DATA_OUTPUT_STATE(m_outputPort, m_outputDataMask, !m_inFault);
            PMUX_DISABLE(m_outputPort, m_outputDataBit);
            tccControlPwm = true;
            modeChangeSuccess = true;

            // Update the LED pattern
            ShiftReg.LedInPwm(m_ledMask,
                              newMode == OUTPUT_H_BRIDGE,
                              m_clearCorePin);
            break;
        default:
            break;
    }

    if (!modeChangeSuccess) {
        return false;
    }

    // Configure PMUX for what needs to be controlled by the TCC.
    if (tccControlPwm) {
        PMUX_ENABLE(m_pwmAInfo->gpioPort, m_pwmAInfo->gpioPin);
        PMUX_ENABLE(m_pwmBInfo->gpioPort, m_pwmBInfo->gpioPin);
    }
    else {
        PMUX_DISABLE(m_pwmAInfo->gpioPort, m_pwmAInfo->gpioPin);
        PMUX_DISABLE(m_pwmBInfo->gpioPort, m_pwmBInfo->gpioPin);
    }

    // Only enable the TCC if it is being used to control something
    if (m_tcc->CTRLA.bit.ENABLE != tccControlPwm) {
        m_tcc->CTRLA.bit.ENABLE = tccControlPwm;
        SYNCBUSY_WAIT(m_tcc, TCC_SYNCBUSY_ENABLE);
    }

    m_mode = newMode;

    return modeChangeSuccess;
}

void DigitalInOutHBridge::FaultState(bool isFaulted) {
    m_inFault = isFaulted;
    // Disable H-bridge driver when in an overload state
    switch (Mode()) {
        case OUTPUT_H_BRIDGE:
        case OUTPUT_WAVE:
        case OUTPUT_TONE:
            DATA_OUTPUT_STATE(m_outputPort, m_outputDataMask, !isFaulted);
            break;
        default:
            break;
    }
}

} // ClearCore namespace