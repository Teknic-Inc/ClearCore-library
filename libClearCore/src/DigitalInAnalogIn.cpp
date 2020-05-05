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
    Connector class for analog and digital inputs.

    This connector class supports reading digital or analog inputs, depending
    on the mode. Connectors of this type include A9 through A12.
**/

#include "DigitalInAnalogIn.h"
#include "AdcManager.h"
#include "StatusManager.h"
#include "SysUtils.h"

namespace ClearCore {

extern ShiftRegister ShiftReg;
extern AdcManager &AdcMgr;
extern StatusManager &StatusMgr;

DigitalInAnalogIn::DigitalInAnalogIn(ShiftRegister::Masks ledMask,
                                     ShiftRegister::Masks modeControlMask,
                                     const PeripheralRoute *inputInfo,
                                     AdcManager::AdcChannels adcChannel)
    : DigitalIn(ledMask, inputInfo),
      m_modeControlBitMask(modeControlMask),
      m_adcChannel(adcChannel),
      m_adcResultConvertedPtr(nullptr),
      m_adcResultConvertedFilteredPtr(nullptr),
      m_analogValid(false) {}

/**
    In digital mode, read the input.
    In analog mode, read the raw value and update the filtered value.
**/
void DigitalInAnalogIn::Refresh() {
    switch (m_mode) {
        case INPUT_ANALOG:
            // If the ShiftRegister was not set up for the analog input
            // when this ADC reading was captured then it is not valid
            if (!(AdcMgr.ShiftRegSnapshot() & m_modeControlBitMask)) {
                // If this is the first valid reading, reset the IIR filter
                if (!m_analogValid) {
                    AdcMgr.FilterReset(m_adcChannel, *m_adcResultConvertedPtr);
                    m_analogValid = true;
                }

                // Set the LED PWM representation of this connector's value
                uint8_t value = (*m_adcResultConvertedPtr) >> 7;
                // Set a lower threshold of when to show any duty on the LED
                if (value < 0x03) {
                    value = 0;
                }
                ShiftReg.LedPwmValue(m_clearCorePin, value);
            }
            break;
        case INPUT_DIGITAL:
            DigitalIn::Refresh();
            break;
        default:
            break;
    }
}

/**
    Reset the analog values, setup the digital input, and
    set the mode to be digital input.
**/
void DigitalInAnalogIn::Initialize(ClearCorePins clearCorePin) {
    // Set the filter time
    AdcMgr.FilterTc(m_adcChannel,
                    ANALOG_INPUT_FILTER_TC_MS_DEFAULT,
                    AdcManager::FILTER_UNIT_MS);

    // Retrieve the pointers to AdcMgr results.
    m_adcResultConvertedPtr = &AdcMgr.ConvertedResult(m_adcChannel);
    m_adcResultConvertedFilteredPtr = &AdcMgr.FilteredResult(m_adcChannel);

    DigitalIn::Initialize(clearCorePin);
    // Mode is now set to INPUT_DIGITAL, reset state is INPUT_ANALOG, switch now
    Mode(INPUT_ANALOG);
}

int16_t DigitalInAnalogIn::State() {
    int16_t state;

    switch (m_mode) {
        case INPUT_ANALOG:
            if (StatusMgr.AdcIsInTimeout()) {
                state = -1;
            }
            else {
                state = *m_adcResultConvertedFilteredPtr >>
                        (15 - AdcMgr.AdcResolution());
            }
            break;
        case INPUT_DIGITAL:
            state = DigitalIn::State();
            break;
        default:
            state = 0;
            break;
    }
    return state;
}

bool DigitalInAnalogIn::FilterTc(uint16_t tc,
                                 AdcManager::FilterUnits theUnits) {
    return AdcMgr.FilterTc(m_adcChannel, tc, theUnits);
}

bool DigitalInAnalogIn::Mode(ConnectorModes newMode) {
    // Bail out if we are already in the requested mode
    if (newMode == m_mode) {
        return true;
    }

    switch (newMode) {
        case INPUT_DIGITAL:
            ShiftReg.ShifterState(true, m_modeControlBitMask);
            // If the system has already been initialized, wait until the
            // digital reading is valid then reset the filtered state
            if (ShiftReg.Ready()) {
                while (!(AdcMgr.ShiftRegSnapshot() & m_modeControlBitMask)) {
                    continue;
                }
                UpdateFilterState();
            }
            ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);
            m_mode = newMode;
            m_analogValid = false;
            break;
        case INPUT_ANALOG:
            ShiftReg.ShifterState(false, m_modeControlBitMask);
            m_mode = newMode;
            // If the system has already been initialized, wait until the analog
            // reading is valid to avoid invalid readings after switching modes
            if (ShiftReg.Ready()) {
                while (!m_analogValid) {
                    continue;
                }
            }
            ShiftReg.LedInPwm(m_ledMask, true, m_clearCorePin);
            break;
        default:
            break;
    }

    return (m_mode == newMode);
}

} // ClearCore namespace
