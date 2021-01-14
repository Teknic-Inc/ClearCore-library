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
    ClearCore Connector Digital Input/Output with Analog Output Class

    This manages an input/output connector on the ClearCore board.

    This class enhances an input point to include an output capability.
**/

#include "DigitalInOutAnalogOut.h"
#include <sam.h>
#include "NvmManager.h"
#include "SysUtils.h"

#define DAC_BITS    11
#define DAC_MAX_VALUE   (UINT16_MAX >> (16 - DAC_BITS))
#define DAC_MAX_OUTPUT_UA   20000
#define DAC_DEFAULT_SPAN    1700

namespace ClearCore {

extern ShiftRegister ShiftReg;
extern NvmManager &NvmMgr;

/**
    Construct, wire in the Input/Output pair, and set pad to input mode.
**/
DigitalInOutAnalogOut::DigitalInOutAnalogOut(
    ShiftRegister::Masks ledMask,
    const PeripheralRoute *inputInfo,
    const PeripheralRoute *outputInfo,
    const PeripheralRoute *outputAnalogInfo,
    bool digitalLogicInversion)
    : DigitalInOut(ledMask, inputInfo, outputInfo, digitalLogicInversion),
      m_analogPort(outputAnalogInfo->gpioPort),
      m_analogDataBit(outputAnalogInfo->gpioPin),
      m_dacZero(0),
      m_dacSpan(DAC_DEFAULT_SPAN) {}

/**
    Do nothing if in analog output mode; otherwise call DigitalInOut's Refresh
**/
void DigitalInOutAnalogOut::Refresh() {
    switch (m_mode) {
        case INPUT_DIGITAL:
        case OUTPUT_DIGITAL:
        case OUTPUT_PWM:
            // Leave the work to the base class
            DigitalInOut::Refresh();
            break;
        case OUTPUT_ANALOG:
        // Do nothing here
        default:
            break;
    }
}

bool DigitalInOutAnalogOut::Mode(ConnectorModes newMode) {
    if (m_mode == newMode) {
        return true;
    }

    switch (newMode) {
        case INPUT_DIGITAL:
        case OUTPUT_DIGITAL:
        case OUTPUT_PWM:
            // The DAC isn't needed in these modes
            DacDisable();
            // Leave the work to the base class
            DigitalInOut::Mode(newMode);
            break;
        case OUTPUT_ANALOG:
            // Analog output requires the output pin to be turned off
            // similar to when we are in digital input mode
            DigitalInOut::Mode(INPUT_DIGITAL);
            // Need the DAC for this mode
            DacEnable();
            m_mode = newMode;
            break;
        default:
            break;
    }

    return (m_mode == newMode);
}

bool DigitalInOutAnalogOut::IsWritable() {
    return m_mode == OUTPUT_DIGITAL || m_mode == OUTPUT_ANALOG ||
           m_mode == OUTPUT_PWM;
}

int16_t DigitalInOutAnalogOut::State() {
    int16_t state;

    switch (m_mode) {
        case OUTPUT_DIGITAL:
        case INPUT_DIGITAL:
        case OUTPUT_PWM:
            // Return internal state, preventing re-read
            state = DigitalInOut::State();
            break;
        default:
            state = 0;
            break;
    }

    return state;
}

bool DigitalInOutAnalogOut::State(int16_t newState) {
    bool success = false;

    switch (m_mode) {
        case INPUT_DIGITAL:
        case OUTPUT_DIGITAL:
        case OUTPUT_PWM:
            success = DigitalInOut::State(newState);
            break;
        case OUTPUT_ANALOG:
            // AnalogWrite takes a uint16_t, so avoid interpreting
            // negative values as large positive values
            newState = max(newState, 0);
            AnalogWrite(newState);
            success = true;
            break;
        default:
            break;
    }

    return success;
}

/**
    Get the DAC and the DigitalInOut ready to go, and get into digital
    input mode.
**/
void DigitalInOutAnalogOut::Initialize(ClearCorePins clearCorePin) {
    // Perform any digital input/output initializing
    DacInitialize();
    DigitalInOut::Initialize(clearCorePin);
    // Mode is now set to INPUT_DIGITAL

    // Initialize the PMUX for analog output, but don't enable the MUX
    PMUX_SELECTION(m_inputPort, m_inputDataBit, PER_ANALOG);   // (-) DAC output
    PMUX_SELECTION(m_analogPort, m_analogDataBit, PER_ANALOG); // (+) DAC output
}

/**
    One-time DAC Configuration
**/
void DigitalInOutAnalogOut::DacInitialize() {
    // Give the DAC a clock
    // The DAC will misbehave if clocked at more than 100 MHz
    SET_CLOCK_SOURCE(DAC_GCLK_ID, 4);

    // Set the supply controller's internal bandgap reference
    SUPC->VREF.bit.SEL = SUPC_VREF_SEL_2V5_Val;

    // Enables the peripheral clock to the DAC
    CLOCK_ENABLE(APBDMASK, DAC_);

    // Reset the DAC module
    DAC->CTRLA.bit.SWRST = 1;
    SYNCBUSY_WAIT(DAC, DAC_SYNCBUSY_SWRST);

    // Use differential mode
    DAC->CTRLB.bit.DIFF = 1;

    // Set to internal bandgap reference
    DAC->CTRLB.bit.REFSEL = DAC_CTRLB_REFSEL_INTREF_Val;

    // Enable the DAC
    DAC->DACCTRL[0].bit.ENABLE = 1;
    // Set refresh rate to (5 x 30) = 150 us
    DAC->DACCTRL[0].bit.REFRESH = 0x5;
    // Current control: CC12M (6 MHz < GCLK_DAC <= 12 MHz)
    DAC->DACCTRL[0].bit.CCTRL = DAC_DACCTRL_CCTRL_CC12M_Val;

    // Write the lowest possible output current so we don't accidentally
    // damage something connected to the pin during setup
    DacRegisterWrite(0);

    DacLoadCalibration();
}

/**
    Turn on the DAC (and possibly cause other I/O functions to misbehave on IO-0)
**/
void DigitalInOutAnalogOut::DacEnable() {
    // Make sure that a valid command has been written to the DAC
    DacRegisterWrite(m_dacZero);

    // Set up the INPUT pin as DAC output
    PMUX_ENABLE(m_inputPort, m_inputDataBit);   // (-) DAC output
    PMUX_ENABLE(m_analogPort, m_analogDataBit); // (+) DAC output

    SYNCBUSY_WAIT(DAC, DAC_SYNCBUSY_ENABLE);
    if (!DAC->CTRLA.bit.ENABLE) {
        DAC->CTRLA.bit.ENABLE = 1;
        SYNCBUSY_WAIT(DAC, DAC_SYNCBUSY_ENABLE);
    }

    while (!DAC->STATUS.vec.READY) {
        continue;
    }

    // Update the LED pattern
    ShiftReg.LedPwmValue(m_clearCorePin, 0);
    ShiftReg.LedInPwm(m_ledMask, true, m_clearCorePin);

    // Set the Cfg00_CFG00_AOUT_BIT to HIGH for use as an Analog Output
    ShiftReg.ShifterStateSet(ShiftRegister::SR_CFG00_AOUT_MASK);
}

/**
    Turn off the DAC
**/
void DigitalInOutAnalogOut::DacDisable() {
    // Set the CFG00_AOUT control point to LOW for use in Digital IO modes
    ShiftReg.ShifterStateClear(ShiftRegister::SR_CFG00_AOUT_MASK);

    // Update the LED pattern
    ShiftReg.LedInPwm(m_ledMask, false, m_clearCorePin);

    // Disable the pin mux
    PMUX_DISABLE(m_inputPort, m_inputDataBit);   // (-) DAC output
    PMUX_DISABLE(m_analogPort, m_analogDataBit); // (+) DAC output

    // Prevent the DAC from operating until re-enabled
    SYNCBUSY_WAIT(DAC, DAC_SYNCBUSY_ENABLE);
    if (DAC->CTRLA.bit.ENABLE) {
        DAC->CTRLA.bit.ENABLE = 0;
        SYNCBUSY_WAIT(DAC, DAC_SYNCBUSY_ENABLE);
    }
}

/**
    Command the DAC to output the given number of microamps
**/
void DigitalInOutAnalogOut::OutputCurrent(uint16_t currentuA) {
    uint16_t dacValue =
        static_cast<uint32_t>(currentuA) * DAC_MAX_VALUE / DAC_MAX_OUTPUT_UA;
    AnalogWrite(dacValue);
}

/**
    Command the DAC to output a raw 12-bit value
**/
void DigitalInOutAnalogOut::AnalogWrite(uint16_t value) {
    if (m_mode != OUTPUT_ANALOG) {
        return;
    }

    value = min(value, DAC_MAX_VALUE);

    // Set the LED blink value
    ShiftReg.LedPwmValue(m_clearCorePin, value * UINT8_MAX / DAC_MAX_VALUE);

    // Factor in calibration
    uint16_t command = ((static_cast<uint32_t>(value) * m_dacSpan)
                        / DAC_MAX_VALUE) + m_dacZero;

    command = min(command, DAC_MAX_VALUE);

    DacRegisterWrite(command);
}

/**
    Load DAC calibration from NVM
**/
void DigitalInOutAnalogOut::DacLoadCalibration() {
    m_dacZero = NvmMgr.Int16(NvmManager::NVM_LOC_DAC_ZERO);
    m_dacSpan = NvmMgr.Int16(NvmManager::NVM_LOC_DAC_SPAN);

    if (m_dacZero > DAC_MAX_VALUE) {
        // Something's wrong with zero, the calibration sketch should be run
        m_dacZero = 0;
    }

    if (m_dacZero + m_dacSpan > DAC_MAX_VALUE) {
        // Something's wrong with the range
        m_dacSpan = DAC_DEFAULT_SPAN - m_dacZero;
    }
}

/**
    write DAC calibration to NVM
**/
bool DigitalInOutAnalogOut::DacStoreCalibration(uint16_t zero, uint16_t span) {
    bool validRange = true;

    if (zero > DAC_MAX_VALUE) {
        validRange = false;
    }

    if (zero + span > DAC_MAX_VALUE) {
        validRange = false;
    }

    if (validRange) {
        // Only write good calibration info
        m_dacZero = zero;
        m_dacSpan = span;

        NvmMgr.Int16(NvmManager::NVM_LOC_DAC_ZERO, m_dacZero);
        NvmMgr.Int16(NvmManager::NVM_LOC_DAC_SPAN, m_dacSpan);
    }

    return validRange;
}

/**
    Write a value to the DAC DATA register.
**/
void DigitalInOutAnalogOut::DacRegisterWrite(uint16_t value) {
    if (DAC->DATA[0].reg != value) {
        SYNCBUSY_WAIT(DAC, DAC_SYNCBUSY_DATA0);
        DAC->DATA[0].reg = value;
    }
}

/**
    Write a value without performing calibration on it.
**/
void DigitalInOutAnalogOut::AnalogWriteUncalibrated(uint16_t value) {
    if (m_mode != OUTPUT_ANALOG) {
        return;
    }

    value = min(value, DAC_MAX_VALUE);

    // Set the LED blink value
    ShiftReg.LedPwmValue(m_clearCorePin, value * UINT8_MAX / DAC_MAX_VALUE);

    DacRegisterWrite(value);
}

} // ClearCore namespace
