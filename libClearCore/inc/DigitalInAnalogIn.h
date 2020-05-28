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
    \file DigitalInAnalogIn.h
    \brief Connector class for analog and digital inputs.
**/
#ifndef __DIGITALINANALOGIN_H__
#define __DIGITALINANALOGIN_H__

#include <stdint.h>
#include <sam.h>
#include "AdcManager.h"
#include "Connector.h"
#include "DigitalIn.h"
#include "IirFilter.h"
#include "PeripheralRoute.h"
#include "ShiftRegister.h"

namespace ClearCore {

/**
    \class DigitalInAnalogIn

    \brief ClearCore analog input connector class.

    This manages an analog input connector on the ClearCore board. This
    connector can also be configured as a digital input.

    The following connector instances support analog input functionality:
    - #ConnectorA9
    - #ConnectorA10
    - #ConnectorA11
    - #ConnectorA12

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/
class DigitalInAnalogIn : public DigitalIn {
    friend class SysManager;

public:
    /**
        Default value for the analog input filter time constant:
        2 milliseconds.
    **/
    static const uint16_t ANALOG_INPUT_FILTER_TC_MS_DEFAULT = 2;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Default constructor so this connector can be a global and constructed
        by SysManager

        \note Should not be called by anything other than SysManager
    **/
    DigitalInAnalogIn() {};
#endif

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (ConnectorA9.Mode() == Connector::INPUT_ANALOG) {
            // A-9 is currently an analog input.
        }
        \endcode

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set the connector's operational mode.

        Set the connector up in the given mode by setting the control bits
        appropriately; also reset the analog filter to the current value if
        setting up for analog mode.

        \code{.cpp}
        // Set A-9's mode to be an analog input
        ConnectorA9.Mode(Connector::INPUT_ANALOG);
        \endcode

        \param[in] newMode The new mode to be set.
        The valid modes for this connector type are:
        - #INPUT_DIGITAL
        - #INPUT_ANALOG.
        \return Returns false if the mode is invalid or setup fails.
    **/
    bool Mode(ConnectorModes newMode) override;

    /**
        \brief Get connector type.

        \code{.cpp}
        if (ConnectorAlias.Type() == Connector::ANALOG_IN_DIGITAL_IN_TYPE) {
            // This generic connector variable is a DigitalInAnalogIn connector
        }
        \endcode

        \return The type of this connector (Analog Input)
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::ANALOG_IN_DIGITAL_IN_TYPE;
    }

    /**
        \brief Is this connector able to be written to?

        \code{.cpp}
        if (!ConnectorAlias.IsWritable()) {
            // This generic connector is not an output
        }
        \endcode

        \return False since this is a read-only connector.
    **/
    bool IsWritable() override {
        return false;
    }

    /**
        \brief Set the time constant for the analog input filter.

        Set the time constant of the analog filter to the given value.

        \code{.cpp}
        // Set A-9's filter time constant to be 10ms
        ConnectorA9.FilterTc(10, AdcManager::FilterUnits::FILTER_UNIT_MS);
        \endcode

        \param[in] tc The time constant to use for the analog filter.
        \param[in] theUnits The units of the time constant \a tc.
        See AdcManager#FilterUnits.

        \return success
    **/
    bool FilterTc(uint16_t tc, AdcManager::FilterUnits theUnits);

    /**
        \brief Get the connector's last majority-filtered sampled value.

        In digital input mode, return the last filtered digital input state.
        When in analog input mode, return the last filtered input value.

        \code{.cpp}
        // Saves A-9's current sampled analog (or digital) input reading
        int16_t analogReading = ConnectorA9.State();
        \endcode

        \return The latest filtered value.
    **/
    int16_t State() override;

    /**
        \brief Returns the analog voltage of the connector in volts.

        \code{.cpp}
        // Saves A-9's current sampled analog input reading in volts
        float analogReadingV = ConnectorA9.AnalogVoltage();
        \endcode

        \return The filtered analog input voltage in volts
    **/
    float AnalogVoltage() {
        // If there is not a valid reading available, return zero.
        if (!m_analogValid) {
            return 0;
        }
        return AdcManager::Instance().AnalogVoltage(m_adcChannel);
    }

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Set the state of a R/W connector.

        Since this is a read-only connector, setting the State has no effect.

        \param[in] newState Does nothing.

        \return failure
    **/
    bool State(int16_t newState) override {
        // ignore attempts to write state
        (void)newState;
        return false;
    }
#endif
private:
    // Control bit for the analog input circuit
    ShiftRegister::Masks m_modeControlBitMask;

    AdcManager::AdcChannels m_adcChannel;

    volatile const uint16_t *m_adcResultConvertedPtr;
    volatile const uint16_t *m_adcResultConvertedFilteredPtr;
    volatile bool m_analogValid;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Construct, wire in pads and LED Shift register object
    **/
    DigitalInAnalogIn(enum ShiftRegister::Masks ledMask,
                      enum ShiftRegister::Masks modeControlMask,
                      const PeripheralRoute *inputInfo,
                      AdcManager::AdcChannels adcChannel);
#endif

    /**
        \brief Update connector's state.

        Poll the underlying connector for new state update.

        This is typically called from a timer or main loop to update the
        underlying value.
    **/
    void Refresh() override;

    /**
        Reset the analog values, set up the digital input, and
        default to digital input mode.

        \param[in] clearCorePin The pin to initialize the connector on.
    **/
    void Initialize(ClearCorePins clearCorePin) override;

}; // DigitalInAnalogIn

} // ClearCore namespace

#endif // __ANALOGINDIGITALIN_H__