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
 *    \file ISerial.h
 *    \brief A standardized ClearCore Serial interface.
 *
 *    Provides a Serial API for USB coms to match standard coms.
 **/


#ifndef ISERIAL_H_
#define ISERIAL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ClearCore {
/**
    \class ISerial
    \brief Base class for interacting with all ClearCore serial.

    This virtual class defines the common functionality for a serial object.

    It provides a generic interface that all serial objects have. This includes
    - Port Opening/Closing
    - Communication configuration
**/
class ISerial {
public:

    /**
        Different types of transmission formats
    **/
    typedef enum _Parities {
        /// Even Parity
        PARITY_E = 0,
        /// Odd Parity
        PARITY_O,
        /// No Parity
        PARITY_N,
    } Parities;

    /**
        Flush the serial port's transmit buffer

        \code{.cpp}
        // Flush COM-0's transmit buffer
        ConnectorCOM0.Flush();
        \endcode
    **/
    virtual void Flush() = 0;
    /**
        Flush the serial port's receive buffer

        \code{.cpp}
        // Flush COM-0's receive buffer
        ConnectorCOM0.FlushInput();
        \endcode
    **/
    virtual void FlushInput() = 0;

    /**
        Set up the port to allow operations

        \code{.cpp}
        // Open COM-0's serial port
        ConnectorCOM0.PortOpen();
        \endcode
    **/
    virtual void PortOpen() = 0;

    /**
        Shut down the port

        \code{.cpp}
        // Close COM-0's serial port
        ConnectorCOM0.PortClose();
        \endcode
    **/
    virtual void PortClose() = 0;

    /**
        \brief Change the baud rate for the port.

        \code{.cpp}
        if (ConnectorCOM0.Speed(9600)) {
            // Setting COM-0's baud to 9600 bps was successful
        }
        \endcode

        \param[in] bitsPerSecond The new speed setting
        \return Returns true if port accepted the speed request.
    **/
    virtual bool Speed(uint32_t bitsPerSecond) = 0;

    /**
        \brief Gets the baud rate of the port.

        \code{.cpp}
        // Saves COM-0's current speed setting
        uint32_t serialSpeed = ConnectorCOM1.Speed();
        \endcode

        \return Returns port speed in bits per second.
    **/
    virtual uint32_t Speed() = 0;

    /**
        \brief Attempt to read the next character from serial channel.

        \return Returns the first character in the serial buffer, or
        SerialBase#EOB if no data are available. If a character is found, it
        will be consumed and removed from the serial buffer.
    **/
    virtual int16_t CharGet() = 0;

    /**
        \brief Attempt to get the next character from the serial channel without
        pulling the character out of the buffer.

        \return Returns the first character in the serial buffer, or
        SerialBase#EOB if no data are available. If a character is found, it
        will not be consumed, and will remain in the serial buffer for reading.
    **/
    virtual int16_t CharPeek() = 0;

    /**
        \brief Send an ascii character on the serial channel.

        \param[in] charToSend The character to be sent
        \return success
    **/
    virtual bool SendChar(uint8_t charToSend) = 0;

    /**
        \brief Send carriage return and newline characters.

        \return success
    **/
    bool SendLine() {
        return SendChar('\r') && SendChar('\n');
    }

    /**
        \brief Send the array of characters out the port.

        \param[in] buffer The array of characters to be sent
        \param[in] bufferSize The number of characters to be sent
        \return success
    **/
    bool Send(const char *buffer, size_t bufferSize) {
        for (size_t iChar = 0; iChar < bufferSize; iChar++) {
            if (!SendChar(buffer[iChar])) {
                return false;
            }
        }
        return true;
    }

    /**
        \brief Send the array of characters out the port.
        Terminate the line with carriage return and newline characters.

        \param[in] buffer The array of characters to be sent
        \param[in] bufferSize The number of characters to be sent
        \return success
    **/
    bool SendLine(const char *buffer, size_t bufferSize) {
        return Send(buffer, bufferSize) && SendLine();
    }

    /**
        \brief Send a string of characters out the port.

        \param[in] nullTermStr The string to be sent
        \return success
    **/
    bool Send(const char *nullTermStr) {
        return Send(nullTermStr, strlen(nullTermStr));
    }

    /**
        \brief Send a string of characters out the port.
        Terminate the line with carriage return and newline characters.

        \param[in] nullTermStr The string to be sent
        \return success
    **/
    bool SendLine(const char *nullTermStr) {
        return Send(nullTermStr) && SendLine();
    }

    /**
        \brief Send a character to be printed to the serial port.

        \param[in] theChar The ascii character to be printed.
        \return success
    **/
    bool Send(char theChar) {
        return SendChar(theChar);
    }

    /**
        \brief Send a character to be printed to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] theChar The ascii character to be printed.
        \return success
    **/
    bool SendLine(char theChar) {
        return Send(theChar) && SendLine();
    }

    /**
        \brief Send a floating point number to the serial port.

        \param[in] number The double precision float value to be printed.
        \param[in] precision (optional) The number of digits to print after the
        decimal point. Default: 2.
        \note The string representation is capped at 20 characters.
        \return success
    **/
    bool Send(double number, uint8_t precision = 2) {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.*f", precision, number);
        return Send(buffer);
    }

    /**
        \brief Send a floating point number to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] number The double precision float value to be printed.
        \param[in] precision (optional) The number of digits to print after the
        decimal point. Default: 2.
        \note The string representation is capped at 20 characters.
        \return success
    **/
    bool SendLine(double number, uint8_t precision = 2) {
        return Send(number, precision) && SendLine();
    }

    /**
        \brief Send an 8-bit signed number to be printed to the serial port.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool Send(int8_t number, uint8_t radix = 10) {
        return Send(static_cast<int32_t>(number), radix);
    }

    /**
        \brief Send an 8-bit signed number to be printed to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool SendLine(int8_t number, uint8_t radix = 10) {
        return SendLine(static_cast<int32_t>(number), radix);
    }

    /**
        \brief Send an 8-bit unsigned number to be printed to the serial port.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool Send(uint8_t number, uint8_t radix = 10) {
        return Send(static_cast<uint32_t>(number), radix);
    }

    /**
        \brief Send an 8-bit unsigned number to be printed to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool SendLine(uint8_t number, uint8_t radix = 10) {
        return SendLine(static_cast<uint32_t>(number), radix);
    }

    /**
        \brief Send a 16-bit signed number to be printed to the serial port.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool Send(int16_t number, uint8_t radix = 10) {
        return Send(static_cast<int32_t>(number), radix);
    }

    /**
        \brief Send a 16-bit signed number to be printed to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool SendLine(int16_t number, uint8_t radix = 10) {
        return SendLine(static_cast<int32_t>(number), radix);
    }

    /**
        \brief Send a 16-bit unsigned number to be printed to the serial port.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool Send(uint16_t number, uint8_t radix = 10) {
        return Send(static_cast<uint32_t>(number), radix);
    }

    /**
        \brief Send a 16-bit unsigned number to be printed to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool SendLine(uint16_t number, uint8_t radix = 10) {
        return SendLine(static_cast<uint32_t>(number), radix);
    }

    /**
        \brief Send a 32-bit signed number to be printed to the serial port.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool Send(int32_t number, uint8_t radix = 10) {
        if (radix < 2 || radix > 16) {
            // Only support bases 2 through 16.
            return false;
        }
        char strRep[2 + 8 * sizeof(number)];
        itoa(number, strRep, radix);
        Send((const char *)strRep);
        return true;
    }

    /**
        \brief Send a 32-bit signed number to be printed to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool SendLine(int32_t number, uint8_t radix = 10) {
        return Send(number, radix) && SendLine();
    }

    /**
        \brief Send a 32-bit unsigned number to be printed to the serial port.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool Send(uint32_t number, uint8_t radix = 10) {
        if (radix < 2 || radix > 16) {
            // Only support bases 2 through 16.
            return false;
        }
        char strRep[1 + 8 * sizeof(number)];
        utoa(number, strRep, radix);
        Send((const char *)strRep);
        return true;
    }

    /**
        \brief Send a 32-bit unsigned number to be printed to the serial port.
        Terminate the line with carriage return and newline characters.

        \param[in] number The value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool SendLine(uint32_t number, uint8_t radix = 10) {
        return Send(number, radix) && SendLine();
    }

    /**
        \brief Send an integer to be printed to the serial port.

        \param[in] number The integer value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool Send(int number, uint8_t radix = 10) {
        return Send(static_cast<int32_t>(number), radix);
    }

    /**
        \brief Send an integer to be printed to the serial port.

        \param[in] number The integer value to be printed.
        \param[in] radix (optional) The base in which to display the character
        representation of \a number. Default: 10.
        \return success
    **/
    bool SendLine(int number, uint8_t radix = 10) {
        return SendLine(static_cast<int32_t>(number), radix);
    }

    /**
        Returns number of characters waiting in the receive buffer.
    **/
    virtual int32_t AvailableForRead() = 0;

    /**
        \brief Determines the number of characters available in the transmit
        buffer.

        \code{.cpp}
        if (ConnectorCOM0.AvailableForWrite() < 10) {
            // There are less than 10 free characters in COM-0's transmit buffer
        }
        \endcode

        \return Returns number of free characters in transmit buffer
    **/
    virtual int32_t AvailableForWrite() = 0;

    /**
        Wait for transmission idle.
    **/
    virtual void WaitForTransmitIdle() = 0;

    /**
        \brief Return whether or not the port is open

        \code{.cpp}
        if (ConnectorCOM0.PortIsOpen()) {
            // COM-0's port is currently open
        }
        \endcode

        \return True if the port is open, and false otherwise.
    **/
    virtual bool PortIsOpen() = 0;

    /**
        Returns whether the serial port is open and the other end is connected.
    **/
    virtual operator bool() = 0;

    /**
        \brief Set UART transmission parity format.

        \code{.cpp}
        if (ConnectorCOM0.Parity(SerialBase::PARITY_O)) {
            // Setting COM-0's parity format to odd was successful
        }
        \endcode

        \param[in] newParity The new parity to be set
        \return Returns true if port accepted the format change request.
    **/
    virtual bool Parity(Parities newParity) = 0;

    /**
        \brief Return current port UART transmission format.

        \code{.cpp}
        if (ConnectorCOM0.Parity() == SerialBase::PARITY_O) {
            // COM-0's parity format is set to odd
        }
        \endcode

        \return Returns transmission format enumeration.
    **/
    virtual Parities Parity() = 0;

    /**
        Change the number of stop bits used in UART communication.

        \code{.cpp}
        if (ConnectorCOM0.StopBits(2)) {
            // Setting COM-0's stop bits was successful
        }
        \endcode
    **/
    virtual bool StopBits(uint8_t bits) = 0;

    /**
        \brief Change the number of bits in a character.

        \code{.cpp}
        if (ConnectorCOM0.CharSize(9)) {
            // Setting COM-0's character size was successful
        }
        \endcode

        \param[in] size The new character size to be set
        \note For UART mode valid settings are: 5,6,7,8,9.
        For SPI mode valid settings are: 8, 9.
    **/
    virtual bool CharSize(uint8_t size) = 0;

};

} // ClearCore namespace

#endif /* ISERIAL_H_ */