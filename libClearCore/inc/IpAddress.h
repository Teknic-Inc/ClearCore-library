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
    \file IpAddress.h

    A wrapper class for an IPv4 IP Address.
**/


#ifndef __IPADDR_H__
#define __IPADDR_H__

#include "lwip/ip_addr.h"

namespace ClearCore {

/**
    \class IpAddress
    \brief An IP Address class.
**/
class IpAddress {

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Construct a default IP Address.

        \code{.cpp}
        IpAddress ip = IpAddress();
        \endcode
    **/
    IpAddress() : m_ipAddress(IPADDR4_INIT(0)) {}
#endif

    /**
        \brief Construct an IP Address from four octet values.

        \code{.cpp}
        IpAddress ip = IpAddress(192, 168, 1, 8);
        \endcode
    **/
    IpAddress(uint8_t firstOctet, uint8_t secondOctet,
              uint8_t thirdOctet, uint8_t fourthOctet) :
        m_ipAddress(IPADDR4_INIT_BYTES(firstOctet, secondOctet,
                                       thirdOctet, fourthOctet)) {}

    /**
        \brief Construct an IP Address from an unsigned integer.

        \code{.cpp}
        IpAddress ip = IpAddress(3232235784);
        \endcode
    **/
    IpAddress(uint32_t ipAddress) :  m_ipAddress(IPADDR4_INIT(ipAddress)) {}

    /**
        \brief Construct an IP Address from a character array.

        \code{.cpp}
        IpAddress ip = IpAddress("192.168.1.8");
        \endcode
    **/
    explicit IpAddress(const char *ipAddress) {
        ipaddr_aton(ipAddress, &m_ipAddress);
    }

    /**
        \brief Returns a string representation of the IP Address.

        \code{.cpp}
        ConnectorCOM0.Send(ip.StringValue());
        \endcode

        \return Returns a string representation of the IP Address.
    **/
    char *StringValue() {
        return ipaddr_ntoa(&m_ipAddress);
    }

#ifndef HIDE_FROM_DOXYGEN
    // Overload cast operators
    operator uint32_t() const {
        return m_ipAddress.addr;
    }
#endif // !HIDE_FROM_DOXYGEN
private:
    ip_addr_t m_ipAddress;

}; // IpAddress

} // ClearCore namespace
#endif // !__IPADDR_H__