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
    \file Connector.h
    \brief Base class for all connector classes.

    This is the base class for interacting with a ClearCore connector.

    It provides a generic interface that all connectors have. This includes
    - Connector Type
    - Generic integer "value"
    - A Refresh function to force the reading of the underlying information
    and perform ancillary work such as scheduling and LED display update.
**/

#ifndef __CONNECTOR_H__
#define __CONNECTOR_H__

#include "SysConnectors.h"
#include <stdint.h>


#ifdef __cplusplus
namespace ClearCore {

/**
    \class Connector
    \brief Base class for interacting with all ClearCore connector objects.

    This virtual class defines the common functionality for a connector object.

    It provides a generic interface that all connectors have. This includes
    - Connector Type
    - Generic integer "state"
    - A Refresh function to force the reading of the underlying information
    and perform ancillary work such as scheduling and LED display update.

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/

class Connector {
    friend class SysManager;

public:
    /**
        \enum ConnectorModes

        \brief All possible operational modes for a connector.

        \note Each type of connector supports only a limited subset of these
        modes.
    **/
    typedef enum {
        /**
            [0] An invalid default mode.
        **/
        INVALID_NONE,
        /**
            [1] Analog input mode.
                \note This is the default mode setting for connectors A-9
                through A-12.
        **/
        INPUT_ANALOG,
        /**
            [2] Digital input mode.
            \note This is the default mode setting for connectors IO-0 through
            DI-8, and all pins on attached CCIO-8 expansion boards.
        **/
        INPUT_DIGITAL,
        /**
            [3] Analog current source output mode.
        **/
        OUTPUT_ANALOG,
        /**
            [4] Digital output mode.
        **/
        OUTPUT_DIGITAL,
        /**
            [5] H-Bridge mode, using differential PWM output.
        **/
        OUTPUT_H_BRIDGE,
        /**
            [6] Periodic digital output mode, using pulse-width modulation (PWM)
        **/
        OUTPUT_PWM,
        /**
            [7] Tone generation mode, using H-Bridge's differential PWM output
            with tone generation features enabled.
        **/
        OUTPUT_TONE,
        /**
            [8] Audio generation mode, playing a wave file from a flash drive.
        **/
        OUTPUT_WAVE,
        /**
            [9] ClearPath&trade; motor controller mode, compatible with
            operational modes that require user's direct control of the A and B
            input signals.
        **/
        CPM_MODE_A_DIRECT_B_DIRECT,
        /**
            [10] ClearPath&trade; motor controller mode, compatible with Step
            and Direction operational modes.
        **/
        CPM_MODE_STEP_AND_DIR,
        /**
            [11] ClearPath&trade; motor controller mode, compatible with
            operational modes where A is controlled by the user directly and B
            is controlled with a PWM signal (e.g., the Follow Digital Torque,
            Velocity, and/or Position commands).
        **/
        CPM_MODE_A_DIRECT_B_PWM,
        /**
            [12] ClearPath&trade; motor controller mode, compatible with Follow
            Digital Velocity: Bipolar PWM Command with Variable Torque
            operational mode where both inputs A and B are controlled with PWM
            signals.
        **/
        CPM_MODE_A_PWM_B_PWM,
        /**
            [13] Serial port mode, using standard TTL levels compatible with
            USB Serial Bridges.
        **/
        TTL,
        /**
            [14] Serial port mode, using inverted TTL levels to allow direct
            RS232 connections for ports tolerant of the lack of negative
            voltages.
        **/
        RS232,
        /**
            [15] Serial port mode, using the port in SPI mode for connections
            to serial devices using this format.
        **/
        SPI,
        /**
            [16] Serial port mode for CCIO-8 connections.
        **/
        CCIO,
        /**
            [17] Serial port mode for USB.
        **/
        USB_CDC
    } ConnectorModes;

    /**
        \enum ConnectorTypes

        \brief The different types of ClearCore connectors.
    **/
    typedef enum {
        /**
            [0] Digital input connector.

            This connector has the following features:
            - Optional majority filtering
            - TTL or 24V input compatibility

            Connectors of this type:
            - ConnectorDI6
            - ConnectorDI7
            - ConnectorDI8
        **/
        DIGITAL_IN_TYPE,
        /**
            [1] Digital input/output connector.

            This connector has the following features:
            - Optional majority input filtering
            - TTL or 24V input compatibility
            - High power digital output

            Connectors of this type:
            - ConnectorIO1
            - ConnectorIO2
            - ConnectorIO3
        **/
        DIGITAL_IN_OUT_TYPE,
        /**
            [2] Virtual connector to access LED and configuration shift register
        **/
        SHIFT_REG_TYPE,
        /**
            [3] Analog and digital input connector.

            This connector supports the following features:
            - Optional majority input filtering
            - TTL or 24V input compatibility
            - 0-10V analog input measurements

            Connectors of this type:
            - ConnectorA9
            - ConnectorA10
            - ConnectorA11
            - ConnectorA12
        **/
        ANALOG_IN_DIGITAL_IN_TYPE,
        /**
            [4] Digital input/output and analog output connector.

            This connector supports the following features:
            - Optional majority input filtering
            - TTL or 24V input compatibility
            - High power digital output
            - 0-20mA analog current output

            Connectors of this type:
            - ConnectorIO0
        **/
        ANALOG_OUT_DIGITAL_IN_OUT_TYPE,
        /**
            [5] H-Bridge connector.

            Utilizing V+ and IO pin as a pair these connectors can be setup
            to:
            - Drive a motor
            - Create tones to drive a speaker
            - Create bi-directional output via PWM

            Connectors of this type:
            - ConnectorIO4
            - ConnectorIO5

            \see DIGITAL_IN_OUT_TYPE
        **/
        H_BRIDGE_TYPE,
        /**
            [6] ClearPath&trade; motor connector.

            This connector can control a ClearPath&trade; motor. Some of the
            control abilities available include:
            - Motor Enable
            - Step+Direction Move Generation
            - Control of the A and B inputs for use with the MC models.
            - Reading of the HLFB from ClearPath motors.

            Connectors of this type:
            - ConnectorM0
            - ConnectorM1
            - ConnectorM2
            - ConnectorM3
        **/
        CPM_TYPE,
        /**
            [7] Serial port connector.

            These connectors can:
            - be used as asynchronous serial ports with selectable baud rate and
              data formats.
            - be used as SPI master ports.

            Connectors of this type:
            - ConnectorCOM0
            - ConnectorCOM1
        **/
        SERIAL_TYPE,
        /**
            [8] Serial USB connector.

            These connectors can:
            - be used as asynchronous serial ports.

            Connectors of this type:
            - ConnectorUsb
        **/
        SERIAL_USB_TYPE,
        /**
            [9] ClearCore I/O Expansion Board digital I/O connector.

            This connector has the following features:
            - Optional majority input filtering
            - TTL or 24V input compatibility
        **/
        CCIO_DIGITAL_IN_OUT_TYPE,
    } ConnectorTypes;

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (ConnectorIO0.Mode() == Connector::OUTPUT_ANALOG) {
            // IO-0 is currently an analog output.
        }
        \endcode

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() {
        return m_mode;
    }

    /**
        \brief Set the connector's operational mode.

        \code{.cpp}
        // Set IO-0's mode to be an analog output
        ConnectorIO0.Mode(Connector::OUTPUT_ANALOG);
        \endcode

        \param[in] newMode The new mode to be set.
        \return Returns false if the mode is invalid or setup fails.
    **/
    virtual bool Mode(ConnectorModes newMode) = 0;

    /**
        \brief Get the connector type.

        \code{.cpp}
        if (ConnectorIO1.Type() == DIGITAL_IN_OUT_TYPE) {
            // IO-1 is a DigitalInOut
        }
        \endcode

        \return Return the connector type for generic handling of connectors.
    **/
    virtual ConnectorTypes Type() = 0;

    /**
        \brief Determine whether values can be written to this connector.

        \code{.cpp}
        if (!ConnectorIO1.IsWritable()) {
            // IO-1 is not currently set as an output
        }
        \endcode

        \return true if this connector is writable, false if this connector is
        read-only.
    **/
    virtual bool IsWritable() = 0;

    /**
        \brief Reinitialize this connector to the power-up state.

        \code{.cpp}
        // IO-1 needs to be re-initialized
        ConnectorIO1.Reinitialize();
        \endcode

        \note Connectors IO-0 through DI-8 and all CCIO-8 connectors will be
        set into #INPUT_DIGITAL mode, while connectors A-9 through A-12 will be
        set into #INPUT_ANALOG mode, the default modes for these connectors.
    **/
    void Reinitialize() {
        Initialize(m_clearCorePin);
    }

    /**
        \brief Accessor for the bit index of this connector in the input
        register.

        \code{.cpp}
        // Save IO-1's index for future use
        int32_t io1Index = ConnectorIO1.ConnectorIndex();
        \endcode

        \return The connector bit index in the input register
    **/
    int32_t ConnectorIndex() {
        return m_clearCorePin;
    }

    /**
        \brief Get the connector's last sampled value.

        Return the current "value" for this connector. For connectors with
        more than one input or output the value returned here would depend
        on the specific connector. Access to this information would need
        to be provided by the implementation object.

        For boolean items, this will return the values of C++ true and false.
        For analog items, this could be the RAW or processed ADC value, etc.

        \code{.cpp}
        if (ConnectorIO0.State()) {
            // IO-0's input is currently high
        }
        \endcode

        \return The latest sample.
    **/
    virtual int16_t State() = 0;

    /**
        \brief Set the state of a R/W connector.

        For read-write objects, this allows you to change the state of the
        connector item.

        \code{.cpp}
        // Set IO-0's output to high
        ConnectorIO0.State(1);
        \endcode

        \param[in] newState For mutable items, update the output.
    **/
    virtual bool State(int16_t newState) = 0;

    /**
        \brief Get whether the connector is in a hardware fault state.

        \code{.cpp}
        if (ConnectorIO1.IsInHwFault()) {
            // IO-1 is in a fault state
        }
        \endcode

        \return True if the connector is in fault; false otherwise.
    **/
    virtual bool IsInHwFault() = 0;

    /**
        \brief Get a bit mask representing this connector.

        \code{.cpp}
        // Create a SysConnectorState mask to check IO-1
        SysConnectorState stateMask(ConnectorIO1.InputRegMask());

        // Save whether IO-1 has risen
        SysConnectorState risenInputs = InputMgr.InputsRisen(stateMask);
        \endcode

        \return A 32-bit mask of 0's, with a single 1 at the bit position of
        this connector's index. (See #ClearCorePins to determine connector
        indices.)
    **/
    uint32_t InputRegMask() {
        return (m_clearCorePin < 0) ? 0 : (1 << m_clearCorePin);
    }

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief This connector's external interrupt line index.

        \note A value of -1 denotes that the connector has no external
        interrupt available.
    **/
    virtual int8_t ExternalInterrupt() {
        return -1;
    }
#endif

protected:
    /**
        Pin for referencing ClearCore pin.

        Set in Initialize().

        Used to communicate with SysManager when the
        Connector's index is required.
    **/
    ClearCorePins m_clearCorePin;

    /**
        Current mode for the connector.
    **/
    ConnectorModes m_mode;

    /// Construct a connector
    Connector();

    /**
        \brief Update the connector's state.

        Poll the underlying connector for new state update.

        This is typically called from a timer or main loop to update the
        underlying value.
    **/
    virtual void Refresh() = 0;

    /**
        \brief Initialize this connector to a "safe" and inert state.

        \param[in] clearCorePin The connector bit index in the input register.
    **/
    virtual void Initialize(ClearCorePins clearCorePin) = 0;

}; // Connector

} // ClearCore namespace
#endif // __cplusplus

#endif // __CONNECTOR_H__