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
    \file SerialUsb.h
    This class controls access to the USB Serial Port Connector.

    Implements ISerial API to be interchangeable with SerialDriver connectors.
**/

#ifndef __SERIALUSB_H__
#define __SERIALUSB_H__

#include <inttypes.h>
#include "Connector.h"
#include "ISerial.h"

/** Decimal (base 10). **/
#ifdef DEC
#undef DEC
#endif
#define DEC 10

/** Hexadecimal (base 16). **/
#ifdef HEX
#undef HEX
#endif
#define HEX 16

/** Octal (base 8). **/
#ifdef OCT
#undef OCT
#endif
#define OCT 8

/** Binary (base 2). **/
#ifdef BIN
#undef BIN
#endif
#define BIN 2

/** Serial USB timeout, in milliseconds (5000ms). **/
#define USB_SERIAL_TIMEOUT 5000 // milliseconds

namespace ClearCore {

/**
    \brief ClearCore Serial USB Connector class

    This class provides support for emulated serial communications on the
    ClearCore's USB port.

    For more detailed information on the ClearCore Connector interface, check
    out the \ref ConnectorMain informational page.
**/
class SerialUsb : public ISerial, public Connector {
    friend class SysManager;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Default constructor so this connector can be a global and
        constructed by SysManager
    **/
    SerialUsb() {};
#endif

    ///////////////////////////////// ISerial API //////////////////////////////

    /**
        \copydoc ISerial::Flush()

        Blocks until data in the write buffer is sent.
    **/
    void Flush() override;

    /**
        \copydoc ISerial::FlushInput()
    **/
    void FlushInput() override;

    /**
        \copydoc ISerial::PortOpen()
    **/
    void PortOpen() override;

    /**
        \copydoc ISerial::PortClose()
    **/
    void PortClose() override;

#ifndef HIDE_FROM_DOXYGEN
    /**
        \copydoc ISerial::Speed(uint32_t bitsPerSecond)
    **/
    bool Speed(uint32_t bitsPerSecond) override;

    /**
        \copydoc ISerial::Speed()
    **/
    uint32_t Speed() override;
#endif

    /**
        \copydoc ISerial::CharGet()
    **/
    int16_t CharGet() override;

    /**
        \copydoc ISerial::CharPeek()
    **/
    int16_t CharPeek() override;

    /**
        \copydoc ISerial::SendChar(uint8_t charToSend)

        \note No characters will be sent if DTR is not asserted.
    **/
    bool SendChar(uint8_t charToSend) override;

    /**
        \copydoc ISerial::AvailableForRead()
    **/
    int32_t AvailableForRead() override;

    /**
        \copydoc ISerial::AvailableForWrite()

        Currently hard-coded to one packet size. While more data can be sent,
        this is the maximum amount of data that will be buffered. Writing
        anything larger will require the data pointer to remain valid during
        the writing.

    **/
    int32_t AvailableForWrite() override;

    /**
        \copydoc ISerial::WaitForTransmitIdle()
    **/
    void WaitForTransmitIdle() override;

    /**
        \copydoc ISerial::PortIsOpen()
    **/
    bool PortIsOpen() override;

    /**
        \brief Set UART transmission parity format.

        \return Returns true if port accepted the format change request.
    **/
    bool Parity(Parities newParity) override {
        return newParity == Parities::PARITY_N;
    }

    /**
        \brief Return current port UART transmission parity.

        \return Returns transmission parity enumeration.
    **/
    Parities Parity() override {
        return Parities::PARITY_N;
    }

    /**
        \brief Change the number of stop bits used in UART communication.

        For USB Serial ports, only 1 stop bit is supported.
    **/
    bool StopBits(uint8_t bits) override {
        return bits == 1;
    }

    /**
        \brief Change the number of bits in a character.

        For USB Serial ports, only 8-bit characters are supported.
    **/
    bool CharSize(uint8_t size) override {
        return size == 8;
    }

    ////////////////////////////// Connector API ///////////////////////////////

    /**
        \brief Get the connector's operational mode.
        \note The only valid mode for this connector is #USB_CDC.

        \return The connector's current operational mode.
    **/
    virtual ConnectorModes Mode() override {
        return Connector::Mode();
    }

    /**
        \brief Set the connector's operational mode.

        \param[in] newMode The new mode to be set.
        The only valid mode is #USB_CDC.
        \return Returns false if the mode is invalid or setup fails.
    **/
    bool Mode(ConnectorModes newMode) override {
        return (m_mode == newMode);
    }

    /**
        \brief Get connector type.

        \return The type of the current connector.
    **/
    Connector::ConnectorTypes Type() override {
        return Connector::SERIAL_USB_TYPE;
    }

    /**
        \brief Get R/W status of the connector.

        \return True if the port is open.
    **/
    bool IsWritable() override {
        return PortIsOpen();
    }

    /**
        \brief Returns whether the serial port is open and the other end is
        connected.

        ClearCore uses the virtual serial port DTR flag to recognize that the
        USB host is connected and listening. If DTR is not asserted, no
        characters will be sent by the SerialUsb Send functions.

         \code{.cpp}
         ConnectorUsb.PortOpen();
         while (!ConnectorUsb) {
             continue;
         }
         ConnectorUsb.Send("USB Serial is now connected and host is listening.");
         \endcode

         \return Returns true if the port is open and DTR is asserted.
    **/
    operator bool() override;

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

private:
    // Index of this instance
    uint16_t m_index;

#ifndef HIDE_FROM_DOXYGEN
    explicit SerialUsb(uint16_t index);

    /**
        Initialize hardware and/or internal state.
    **/
    void Initialize(ClearCorePins clearCorePin) override {
        m_index = clearCorePin;
        m_mode = USB_CDC;
    }

    /**
        \brief Update connector's state.

        \return Update the internal state.
    **/
    void Refresh() override {};
#endif
}; // SerialUsb

} // ClearCore namespace

#endif // __SERIALDRIVER_H__