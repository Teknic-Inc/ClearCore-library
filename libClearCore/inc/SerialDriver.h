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
    \file SerialDriver.h
    This class controls access to the Serial Port Connector.

    It will allow you to set up:
        - RS232 direct connections for ports that tolerate no negative voltages
          by inverting the serial signals.
        - TTL direct connections to USB bridge parts
        - SPI transfers
**/

#ifndef __SERIALDRIVER_H__
#define __SERIALDRIVER_H__

#include <stdint.h>
#include "Connector.h"
#include "SerialBase.h"
#include "ShiftRegister.h"

namespace ClearCore {

/**
    \brief ClearCore Serial UART/SPI Connector class

    This class controls access to the COM-0 and COM-1 connectors.

    For more detailed information on each configurable mode, see the
    \ref SerialDriverMain informational page.

    For more detailed information on the ClearCore Connector interface in
    general, check out the \ref ConnectorMain informational page.
**/
class SerialDriver : public SerialBase, public Connector {
    friend class SysManager;
    friend class CcioBoardManager;
    friend class TestIO;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Default constructor so this connector can be a global and
        constructed by SysManager.
    **/
    SerialDriver() {};
#endif

    /**
        \brief Get the connector's operational mode.

        \code{.cpp}
        if (ConnectorCOM0.Mode() == Connector::TTL) {
            // COM-0 is currently in TTL mode.
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
        // Set COM-1's mode to be the port controlling a CCIO-8 link
        ConnectorCOM1.Mode(Connector::CCIO);
        \endcode

        \param[in] newMode The new mode to be set.
        The valid modes for this connector type are:
        - Connector#RS232
        - Connector#SPI
        - Connector#TTL
        - Connector#CCIO
        \return Returns false if the mode is invalid or setup fails.
    **/
    bool Mode(ConnectorModes newMode) override;

    /**
        \brief Get connector type.

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::SERIAL_TYPE;
    };

    /**
        \brief Get R/W status of the connector.

        \return True if the port is open.
    **/
    bool IsWritable() override {
        return PortIsOpen();
    };

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Alias to ISerial::PortIsOpen().

        \return True if the port is open, and false otherwise.
    **/
    int16_t State() override {
        return PortIsOpen();
    }

    /**
        \brief Alternative method to open or close the serial port.

        \param[in] newState Zero to close the port, non-zero to open the port.

        \return Success
    **/
    bool State(int16_t newState) override {
        if (newState) {
            PortOpen();
        }
        else {
            PortClose();
        }
        return true;
    }

    bool IsInHwFault() override {
        return false;
    }
#endif

    /**
        \brief Change the baud rate for the port.

        \param[in] bitsPerSecond The new speed to set.
        \return Returns true if port accepted the speed request. Will return
        false if the baud rate gets clipped for SPI mode.
    **/
    bool Speed(uint32_t bitsPerSecond) override {
        bool retVal = SerialBase::Speed(bitsPerSecond);
        // Delay to allow the port polarity to be written to the shift
        // register and settle for a full character time before sending data
        if (m_portOpen) {
            WaitOneCharTime();
        }
        return retVal;
    }

    /**
        Set up the port to allow operations/communications.
    **/
    void PortOpen() override;

    /**
        Shut down the port and discontinue operations/communications.
    **/
    void PortClose() override;

private:
    // Index of this instance
    uint8_t m_index;
    // Feedback LED
    ShiftRegister::Masks m_ledMask;
    // Control bit in shift register
    ShiftRegister::Masks m_controlMask;
    // Inverter bit in shift register
    ShiftRegister::Masks m_polarityMask;

#ifndef HIDE_FROM_DOXYGEN
    /**
        Construct and wire in LED bit number
    **/
    SerialDriver(uint16_t index,
                 ShiftRegister::Masks feedBackLedMask,
                 ShiftRegister::Masks controlMask,
                 ShiftRegister::Masks polarityMask,
                 const PeripheralRoute *ctsMisoInfo,
                 const PeripheralRoute *rtsSsInfo,
                 const PeripheralRoute *rxSckInfo,
                 const PeripheralRoute *txMosiInfo,
                 uint8_t peripheral);
#endif
    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins clearCorePin) override;

    /**
        Update connector's state.
    **/
    void Refresh() override {};
}; // SerialDriver

} // ClearCore namespace

#endif // __SERIALDRIVER_H__