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
    \file DigitalInOut.h

    \brief ClearCore Digital Output Connector class
**/

#ifndef __DIGITALINOUT_H__
#define __DIGITALINOUT_H__

#include <stdint.h>
#include <sam.h>
#include "Connector.h"
#include "DigitalIn.h"
#include "ShiftRegister.h"
#include "SysUtils.h"

namespace ClearCore {

/**
    \class DigitalInOut

    \brief ClearCore digital output connector class.

    This manages a digital output connector on the ClearCore board. This
    connector can also be configured as a digital input.

    The following connector instances support digital output functionality:
    - #ConnectorIO0
    - #ConnectorIO1
    - #ConnectorIO2
    - #ConnectorIO3
    - #ConnectorIO4
    - #ConnectorIO5

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/
class DigitalInOut : public DigitalIn {
    friend class SysManager;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        Default constructor so this connector can be a global and constructed
        by SysManager

        \note Should not be called by anything other than SysManager
    **/
    DigitalInOut() {};
#endif

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (ConnectorIO1.Mode() == Connector::OUTPUT_DIGITAL) {
            // IO-1 is currently a digital output.
        }
        \endcode

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set the connector's operational mode.

        \code{.cpp}
        // Set IO-1's mode to be an output configured to produce PWM signals
        ConnectorIO1.Mode(Connector::OUTPUT_PWM);
        \endcode

        \param[in] newMode The new mode to be set.
        The valid modes for this connector type are:
        - #INPUT_DIGITAL
        - #OUTPUT_DIGITAL
        - #OUTPUT_PWM.
        \return Returns false if the mode is invalid or setup fails.
    **/
    virtual bool Mode(ConnectorModes newMode) override;

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias.Type() == Connector::DIGITAL_IN_OUT_TYPE) {
            // This generic connector variable is a DigitalInOut connector
        }
        \endcode

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::DIGITAL_IN_OUT_TYPE;
    }

    /**
        \brief Get R/W status of the connector.

        \code{.cpp}
        if (ConnectorIO1.IsWritable()) {
            // IO-1 is in an output mode
        }
        \endcode

        \return True if in #OUTPUT_DIGITAL or #OUTPUT_PWM mode, false otherwise.
    **/
    bool IsWritable() override {
        return m_mode == OUTPUT_DIGITAL || m_mode == OUTPUT_PWM;
    }

    /**
        \brief Get the connector's last majority-filtered sampled value.

        \code{.cpp}
        // In this example, IO-1 has been configured for digital input
        if (ConnectorIO1.State()) {
            // IO-1's input is currently high
        }
        \endcode

        \code{.cpp}
        // In this example, IO-1 has been configured for PWM output
        if (ConnectorIO1.State() > UINT8_MAX / 2) {
            // IO-1 is outputting PWM at a duty cycle greater than 50%
        }
        \endcode

        \return The latest filtered value on this connector.

        \note If the filter length is set to 0, this will return the real-time
        value from the hardware register instead (see
        #FilterLength(uint16_t samples)).
    **/
    int16_t State() override;

    /**
        \brief Set the output state of the connector.

        This allows you to change the output value of the connector item.

        \code{.cpp}
        // Configure IO-1 for digital output mode
        ConnectorIO1.Mode(Connector::OUTPUT_DIGITAL);
        // Set IO-1's output to high
        ConnectorIO1.State(1);
        \endcode

        \code{.cpp}
        // Configure IO-1 for PWM output mode
        ConnectorIO1.Mode(Connector::OUTPUT_PWM);
        // Set IO-1 to output a PWM wave with 25% duty cycle
        ConnectorIO1.State(UINT8_MAX / 4);
        \endcode

        \param[in] newState The value to be output.
    **/
    bool State(int16_t newState) override;

    /**
        \copydoc Connector::IsInHwFault()
    **/
    bool IsInHwFault() override {
        return (volatile bool &)m_isInFault;
    }

    /**
        \brief Start an output pulse.

        This allows you to start a pulse on the output that is on for \a onTime
        milliseconds and off for \a offTime milliseconds and will stop after
        \a pulseCount cycles. A \a pulseCount of 0 will cause the pulse to run
        endlessly. If a pulse is already running, calling this will allow you
        to override the previous pulse (after the next change in state).

        \code{.cpp}
        // Begin a 100ms on/200ms off pulse on IO-1's output that will complete
        // 20 cycles and prevent further code execution until the cycles are
        // complete
        ConnectorIO1.OutputPulsesStart(100, 200, 20, true);
        \endcode

        \param[in] onTime The amount of time the input will be held on [ms].
        \param[in] offTime The amount of time the input will be held off [ms].
        \param[in] pulseCount (optional) The amount of cycles the pulse will
        run for. Default: 0 (pulse runs endlessly).
        \param[in] blockUntilDone (optional) If true, the function call will
        wait until \a pulseCount pulses have been sent before returning.
        Default: false.
    **/
    void OutputPulsesStart(uint32_t onTime, uint32_t offTime,
                           uint16_t pulseCount = 0,
                           bool blockUntilDone = false);

    /**
        \brief Stop an output pulse.

        This allows you to stop the currently running pulse on this output. The
        output will always be set to FALSE after canceling a pulse.

        \code{.cpp}
        // Stop the active output pulse on IO-1
        ConnectorIO1.OutputPulsesStop();
        \endcode

        \param[in] stopImmediately (optional) If true, the output pulses will
        be stopped immediately; if false, any active pulse will be completed
        first. Default: true.
    **/
    void OutputPulsesStop(bool stopImmediately = true);

    /**
        \brief Check the output pulse state.

        This allows you to see if there is a currently running pulse on this
        output.

        \code{.cpp}
        if (ConnectorIO1.OutputPulsesActive()) {
            // IO-1 is outputting pulses
        }
        \endcode

        \return True if output pulses are currently active.
    **/
    volatile const bool &OutputPulsesActive() {
        return m_pulseActive;
    }

    /**
        \brief Set the PWM duty on the I/O pin.

        The pin must be in #OUTPUT_PWM mode, or else nothing will happen and
        this function will return false.

        \code{.cpp}
        // Configure IO-1 for PWM output
        ConnectorIO1.Mode(Connector::OUTPUT_PWM);
        // Set the PWM output signal on IO-1 to be asserted 25% of the time
        ConnectorIO1.PwmDuty(UINT8_MAX / 4);
        \endcode

        \param[in] newDuty The PWM duty cycle for the pin, from 0 to
        255 (UINT8_MAX).

        \return Successfully set the PWM duty value.
    **/
    bool PwmDuty(uint8_t newDuty);

protected:
    // Port access
    uint32_t m_outputPort;
    uint32_t m_outputDataBit;
    uint32_t m_outputDataMask;
    bool m_logicInversion;

    bool m_isInFault;

    Tc *m_tc;
    uint8_t m_tcPadNum;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Construct and wire in the Input/Output pair.
    **/
    DigitalInOut(enum ShiftRegister::Masks ledMask,
                 const PeripheralRoute *inputInfo,
                 const PeripheralRoute *outputInfo,
                 bool logicInversion);

    /**
        \brief Update connector's state.

        Poll the underlying connector for new state update.

        This is typically called from a timer or main loop to update the
        underlying value.
    **/
    void Refresh() override;

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins clearCorePin) override;
#endif
private:
    bool m_outState;
    // Pulse control variables
    uint32_t m_pulseOnTicks;
    uint32_t m_pulseOffTicks;
    uint32_t m_pulseStart;
    uint16_t m_pulseStopCount;
    uint16_t m_pulseCounter;
    uint8_t m_overloadTripCnt;
    bool m_pulseActive;
    bool m_pulseValue;
    bool m_pulseStopPending;
    uint16_t m_overloadFoldbackCnt;

    void OutputPin(bool val) {
        DATA_OUTPUT_STATE(m_outputPort, m_outputDataMask,
                          val != m_logicInversion);
    }

    /**
        \brief Sets whether the connector is in a hardware fault state.

        \param[in] inFault Fault state to set
    **/
    void IsInHwFault(bool inFault);
};

} // ClearCore namespace

#endif // __DIGITALINOUT_H__