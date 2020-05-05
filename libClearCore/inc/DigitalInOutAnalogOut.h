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
    \file DigitalInOutAnalogOut.h

    \brief ClearCore Analog Output Connector class
**/

#ifndef __DIGITALINOUTANALOGOUT_H__
#define __DIGITALINOUTANALOGOUT_H__

#include <stdint.h>
#include "Connector.h"
#include "DigitalInOut.h"
#include "PeripheralRoute.h"
#include "ShiftRegister.h"

namespace ClearCore {

/**
    \brief ClearCore digital input/output with analog current output
    Connector class.

    This manages an analog output connector on the ClearCore board. This
    connector can also be configured as a digital output or digital input.

    The following connector instances support analog output functionality:
    - #ConnectorIO0

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/
class DigitalInOutAnalogOut : public DigitalInOut {
    friend class SysManager;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Default constructor so this connector can be a global and
        constructed by SysManager

        \note Should not be called by anything other than SysManager
    **/
    DigitalInOutAnalogOut() {};
#endif

    /**
        \copydoc Connector::Mode
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set connector's operational mode.

        \code{.cpp}
        // Set IO-0's mode to be an analog output
        ConnectorIO0.Mode(Connector::OUTPUT_ANALOG);
        \endcode

        \param[in] newMode The new operational mode to be set.
        The valid modes for this connector type are:
        - #INPUT_DIGITAL
        - #OUTPUT_DIGITAL
        - #OUTPUT_PWM
        - #OUTPUT_ANALOG.
        \return True if the mode change was successful.
    **/
    bool Mode(ConnectorModes newMode) override;

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias.Type() ==
                Connector::ANALOG_OUT_DIGITAL_IN_OUT_TYPE) {
            // This generic connector variable is a DigitalInOutAnalogOut
            // connector
        }
        \endcode

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::ANALOG_OUT_DIGITAL_IN_OUT_TYPE;
    }

    /**
        \brief Get R/W status of the connector.

        \code{.cpp}
        if (ConnectorIO0.IsWritable()) {
            // IO-0 is in output mode
        }
        \endcode

        \return False if the connector is read-only.
    **/
    bool IsWritable() override;

    /**
        \brief Get connector's last sampled digital value.

        In digital modes, return the current digital state; in analog
        output mode, returns 0.

        \code{.cpp}
        if (ConnectorIO0.State()) {
            // IO-0's input is currently high
        }
        \endcode

        \return 0 when in analog output mode; the current digital state
                otherwise
    **/
    int16_t State() override;

    /**
        \brief Set the output value of the connector.

        When in digital modes, set the digital output value via DigitalInOut
        class. When in analog output mode, write the analog value out. Valid
        analog values are unsigned 11-bit integers, where 0 corresponds to 0 mA
        (minimum current output) and 2047 corresponds to 20 mA (maximum current
        output).

        \code{.cpp}
        // Configure IO-0 for analog output
        ConnectorIO0.Mode(Connector::OUTPUT_ANALOG);
        // Write a (scaled) analog value to IO-0's output
        ConnectorIO0.State(50);
        \endcode

        \param[in] newState The value to be output
    **/
    bool State(int16_t newState) override;

    /**
        \brief Command the DAC to output a (calibrated) value.

        A value of 2047 corresponds to maximum (20 mA) current output, and
        a value of 0 corresponds to minimum (0 mA) current output.

        \code{.cpp}
        // Configure IO-0 for analog output
        ConnectorIO0.Mode(Connector::OUTPUT_ANALOG);
        // Output the maximum analog voltage to IO-0
        ConnectorIO0.AnalogWrite(2047);
        \endcode

        \param[in] value The raw 11-bit DAC value to command analog current.
    **/
    void AnalogWrite(uint16_t value);

    /**
        \brief Command the DAC to output the given number of microamps (uA).

        \code{.cpp}
        // Configure IO-0 for analog output
        ConnectorIO0.Mode(Connector::OUTPUT_ANALOG);
        // Output 5000 uA (5 mA) to IO-0's analog output
        ConnectorIO0.OutputCurrent(5000);
        \endcode

        \param[in] currentuA Output current command in microamps (uA).
    **/
    void OutputCurrent(uint16_t currentuA);

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief This function should only be used for calibration purposes.

        Command the DAC to output a raw uncalibrated value between 0
        and 2047. 0 corresponds to the hardware's minimum current output (0 mA),
        and 2047 corresponds to the hardware's maximum current output (20 mA).

        \param[in] value The raw 11-bit DAC value to command analog current.
    **/
    void AnalogWriteUncalibrated(uint16_t value);

    /**
        \brief Persist the DAC's calibration setting in NVM.

        Warning: Calling this function WILL overwrite any existing ClearCore
        factory configuration. Improper use of this function will result in
        poorly calibrated DAC output.

        The \a zero parameter sets the minimum output value, corresponding to
        minimum current output.
        The \a span parameter sets the range of valid output values.
        In essence, the DAC's output is scaled between \a zero  and
        (\a zero + \a span).

        \param[in] zero The 11-bit zero value (will be clipped if the value
        exceeds 2047)
        \param[in] span The 11-bit span value (clipped)
        \return True if the calibration data is written successfully;
        false otherwise.
    **/
    bool DacStoreCalibration(uint16_t zero, uint16_t span);
#endif // HIDE_FROM_DOXYGEN

private:
    uint32_t m_analogPort;
    uint32_t m_analogDataBit;
    uint16_t m_dacZero;
    uint16_t m_dacSpan;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Construct and wire in the Input/Output pair.
    **/
    DigitalInOutAnalogOut(enum ShiftRegister::Masks ledMask,
                          const PeripheralRoute *inputInfo,
                          const PeripheralRoute *outputInfo,
                          const PeripheralRoute *outputAnalogInfo,
                          bool digitalLogicInversion);
#endif
    /**
        \brief Update connector's state.

        For this connector, the Refresh does nothing when in analog output
        mode; otherwise, the work is handled by the parent #DigitalInOut class.

        This is typically called from a timer or main loop to update the
        underlying value.
    **/
    void Refresh() override;

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins clearCorePin) override;

    /**
        One-time set up required to use the DAC on #ConnectorIO0.
        This should be run before #DacEnable() or #DacDisable().
    **/
    void DacInitialize();

    /**
        Command the DAC to start outputting voltage. If the DAC is
        enabled, other I/O functions on the pin may not behave properly.
    **/
    void DacEnable();

    /**
        Command the DAC to stop outputting voltage. Do this before using
        other I/O functions on the pin.
    **/
    void DacDisable();

    /**
        Load DAC calibration values (zero, span) from NVM.
    **/
    void DacLoadCalibration();

    /**
        \brief Write a value to the DAC's DATA register.

        Warning: No operations are performed on the input value so use at your
        own risk, and be sure that \a value does not exceed 2047.

        \param[in] value The 11-bit command to send
    **/
    void DacRegisterWrite(uint16_t value);

}; // DigitalInOutAnalogOut class

} // ClearCore namespace

#endif // __ANALOGOUTDIGITALINOUT_H__
