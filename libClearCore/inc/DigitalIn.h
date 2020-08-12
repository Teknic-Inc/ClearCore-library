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
    \file DigitalIn.h

    \brief ClearCore Digital Input Connector class.
**/

#ifndef __DIGITALIN_H__
#define __DIGITALIN_H__

#include <stdint.h>
#include "Connector.h"
#include "InputManager.h"
#include "PeripheralRoute.h"
#include "ShiftRegister.h"

namespace ClearCore {

/**
    Pointer to a function that takes no parameters and returns nothing.
**/
typedef void (*voidFuncPtr)(void);

/**
    \class DigitalIn

    \brief ClearCore digital input connector class.

    This manages a digital input connector on the ClearCore board.

    The following connector instances support digital input functionality:
    - #ConnectorIO0
    - #ConnectorIO1
    - #ConnectorIO2
    - #ConnectorIO3
    - #ConnectorIO4
    - #ConnectorIO5
    - #ConnectorDI6
    - #ConnectorDI7
    - #ConnectorDI8
    - #ConnectorA9
    - #ConnectorA10
    - #ConnectorA11
    - #ConnectorA12

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/
class DigitalIn : public Connector {
    friend class SysManager;
    friend class TestIO;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        Default constructor so this connector can be a global and constructed
        by SysManager

        \note Should not be called by anything other than SysManager
    **/
    DigitalIn() {};
#endif

    /**
        \enum FilterUnits
        \brief Units for the digital filter length.

        \note One sample time is 200 microseconds, so 1 ms = 5 sample times.
    **/
    typedef enum {
        /** Milliseconds. **/
        FILTER_UNIT_MS,
        /** Sample times. **/
        FILTER_UNIT_SAMPLES,
    } FilterUnits;

    /**
        \brief Set the connector's digital transition filter length.
        The default digital filter length for digital input connectors is 3
        samples.

        This will set the length, in samples (default) or milliseconds, of the
        connector's transition filter and restarts any filtering in progress.

        \code{.cpp}
        // Sets DI-6's filter to 20 samples (4ms)
        ConnectorDI6.FilterLength(20);
        \endcode

        \code{.cpp}
        // Sets DI-6's filter to 10ms (50 samples)
        ConnectorDI6.FilterLength(10, DigitalIn::FILTER_UNIT_MS);
        \endcode

        \note One sample time is 200 microseconds.

        \param[in] length The length of the filter.
        \param[in] units The units of the specified filter length: samples
        (default) or milliseconds.
    **/
    void FilterLength(uint16_t length,
                      FilterUnits units = FILTER_UNIT_SAMPLES) {
        // 1 ms = 1000 us = 5 * (200 us) = 5 sample times
        uint16_t samples = (units == FILTER_UNIT_MS) ? 5 * length : length;
        m_filterLength = samples;
        m_filterTicksLeft = samples;
    }

    /**
        \brief Get the connector's digital filter length in samples. The default
        is 3 samples.

        This will get the length, in samples, of the connector's filter.

        \code{.cpp}
        if (ConnectorDI6.FilterLength() > 5) {
            // DI6's filter length is greater than 5 samples (1ms), do something
        }
        \endcode

        \note One sample time is 200 microseconds.

        \return The length of the digital input filter in samples.
    **/
    uint16_t FilterLength() {
        return m_filterLength;
    }

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (ConnectorDI6.Mode() == Connector::INPUT_DIGITAL) {
            // DI-6 is currently a digital input.
        }
        \endcode

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set the connector's operational mode.

        \param[in] newMode The new mode to be set.
        The only valid mode for this connector type is: #INPUT_DIGITAL.
        \return Returns false if the mode is invalid or setup fails.
    **/
    virtual bool Mode(ConnectorModes newMode) override {
        if (newMode == ConnectorModes::INPUT_DIGITAL) {
            m_mode = newMode;
            return true;
        }
        else {
            return false;
        }
    }

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias.Type() == Connector::DIGITAL_IN_TYPE) {
            // This generic connector variable is a DigitalIn connector
        }
        \endcode

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::DIGITAL_IN_TYPE;
    }

    /**
        \brief Get R/W status of the connector.

        \code{.cpp}
        if (!ConnectorAlias.IsWritable()) {
            // This generic connector is not an output
        }
        \endcode

        \return False because the connector is read-only.
    **/
    bool IsWritable() override {
        return false;
    }

    /**
        \brief Get the connector's last majority-filtered sampled value.

        \code{.cpp}
        if (ConnectorDI6.State()) {
            // DI-6's input is currently high
        }
        \endcode

        \return The latest filtered value on this connector.

        \note If the filter length is set to 0, this will return the real-time
        value from the hardware register instead (see
        #FilterLength(uint16_t samples)).
    **/
    int16_t State() override;

#ifndef HIDE_FROM_DOXYGEN

    /**
        \brief Get the connector's real time input value.

        \return The current value on this connector.
    **/
    int16_t StateRT();

    /**
        \brief Set the state of a R/W connector.

        For read-write objects, this allows you to change the state of the
        connector item.

        \param[in] newState For mutable items, update the output.

        \note Does nothing since digital input connectors have no output
        capability.
    **/
    bool State(int16_t newState) override {
        (void)newState;
        return false;
    }
#endif

    /**
        \brief Clear on read accessor for this connector's rising input state.

        \code{.cpp}
        if (ConnectorDI7.InputRisen()) {
            // DI-7 rising edge detected since the last call
        }
        \endcode

        \return true if the input has risen since the last call
    **/
    bool InputRisen();

    /**
        \brief Clear on read accessor for this connector's falling input state.

        \code{.cpp}
        if (ConnectorDI7.InputFallen()) {
            // DI-7 falling edge detected since the last call
        }
        \endcode

        \return true if the input has fallen since the last call
    **/
    bool InputFallen();

#ifndef HIDE_FROM_DOXYGEN
    /**
        \copydoc Connector::IsInHwFault()
        \note Since this connector can only be configured as an input,
        a fault state is not possible and so this function will always
        return false.
    **/
    bool IsInHwFault() override {
        return false;
    }

    /**
        \brief This connector's external interrupt line index.

        \note A value of -1 denotes that the connector has no external
        interrupt available.
    **/
    int8_t ExternalInterrupt() override {
        return m_extInt;
    }
#endif

    /**
        \brief Register the interrupt service routine to be triggered when the
        given input state condition is met on this connector.

        \code{.cpp}
        // Signature for interrupt service routine (ISR)
        void myCallback();

        void setup() {
            // Set an interrupt service routine (ISR) to be called when the
            // state of the interrupt connector goes from TRUE to FALSE.
            InterruptConnector.InterruptHandlerSet(&myCallback,
                                                   InputManager::FALLING);

            // Enable digital interrupts.
            InputMgr.InterruptsEnabled(true);
        }

        // The function to be triggered on an interrupt.
        // This function blinks the user-controlled LED once.
        void myCallback() {
            ConnectorLed.State(true);
            Delay_ms(100);
            ConnectorLed.State(false);
            Delay_ms(100);
        }
        \endcode

        \param[in] callback The ISR to be called when the interrupt is triggered
        \param[in] trigger The input state condition on which to trigger the
        interrupt.
        \param[in] enable Whether this interrupt should be immediately enabled.
        \return true if the ISR was registered successfully, false otherwise.

        \note Only connectors DI-6 through A-12 can trigger interrupts.
    **/
    bool InterruptHandlerSet(voidFuncPtr callback = nullptr,
                             InputManager::InterruptTrigger trigger =
                                 InputManager::InterruptTrigger::RISING,
                             bool enable = true);

    /**
        \brief Enable or disable the interrupt on this connector.

        \code{.cpp}
        // Enable interrupts on DI-6
        ConnectorDI6.InterruptEnable(true);
        \endcode

        \param[in] enable If true, enable the interrupt. If false, disable the
        interrupt.

        \note Only connectors DI-6 through A-12 can trigger interrupts.
        \note InputManager has a global version of this function that enables
        or disables the interrupts for all connectors.
    **/
    void InterruptEnable(bool enable);

protected:
    // LED associated with input
    ShiftRegister::Masks m_ledMask;

    // Register that contains the digital input
    uint32_t m_inputPort;
    uint32_t m_inputDataBit;
    uint32_t m_inputDataMask;

    // External interrupts
    uint8_t m_extInt;        // External interrupt line index
    bool m_interruptAvail;   // An external interrupt is available on this input

    uint32_t *m_changeRegPtr;
    uint32_t *m_inRegPtr;
    uint32_t *m_inputRegRTPtr;

    // Boolean state holders
    bool m_stateFiltered;

    // Stability filter
    uint16_t m_filterLength;
    // Set to filter length on input state change
    uint16_t m_filterTicksLeft;

    /**
        Construct, wire in pads and LED shift register object.
    **/
    DigitalIn(enum ShiftRegister::Masks ledMask,
              const PeripheralRoute *inputInfo);

    /**
        \brief Update the connector's state.

        Poll the underlying connector for a state update.

        This is typically called from a timer or main loop to update the
        underlying value.

        \return Updated filtered internal state.
    **/
    void Refresh() override;

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins clearCorePin) override;

    /**
        Set the filtered pin value to the current input register state.
    **/
    void UpdateFilterState();

}; // DigitalIn

} // ClearCore namespace

#endif // __DIGITALIN_H__