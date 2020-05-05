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
    \file EthernetTcpClient.h
**/

#ifndef __ETHERNETTCPCLIENT_H__
#define __ETHERNETTCPCLIENT_H__

#include <stdint.h>
#include "EthernetTcp.h"
#include "IpAddress.h"

namespace ClearCore {

/** The minimum TCP connection timeout value, in milliseconds. Attempts to set a
    TCP connection timeout less than this value will fail, and this value will
    be used instead. **/
#define TCP_CONNECTION_TIMEOUT_MIN 100
/** The maximum TCP connection timeout value, in milliseconds. Attempts to set a
    TCP connection timeout greater than this value will fail, and this value
    will be used instead. **/
#define TCP_CONNECTION_TIMEOUT_MAX 15000

/**
    \brief ClearCore TCP client class.

    This class manages interactions with a single Ethernet TCP client
    connection.

    For more detailed information on the ClearCore Ethernet system, check out
    the \ref EthernetMain informational page.
**/
class EthernetTcpClient : public EthernetTcp {

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        Construct an empty TCP client with no existing connection information.
    **/
    EthernetTcpClient();

    /**
        Construct a TCP client with existing connection information.
    **/
    explicit EthernetTcpClient(TcpData *tcpData);
#endif // !HIDE_FROM_DOXYGEN

    /**
        \brief Connects the client to a specified remote IP address and port.

        \return True if the connection was successful.
    **/
    bool Connect(IpAddress ip, uint16_t port);

    /**
        \brief Determines if the client is actively connected to a server.

        \return True if the client's connection is active. False if there is no
        connection or the existing connection is closed or closing.
    **/
    bool Connected();

    /**
        \brief Returns the number of bytes available to read.

        \return The number of bytes available to read.
    **/
    int16_t BytesAvailable();

    /**
        \brief Attempt to read the next available character.

        Attempts to get the next available character from the client's
        incoming data buffer.

        \return The character first character available, or -1 if no data is
        available.
    **/
    int16_t Read();

    /**
        \brief Reads data received from the server.

        \param[out] dataPtr A pointer to a buffer to hold the received data.
        \param[in] length The maximum number of bytes to read.

        \return The number of bytes read.
    **/
    int16_t Read(uint8_t *dataPtr, uint32_t length);

    /**
        \brief Attempt to get the next available character.

        Attempts to get the next available character without pulling the
        character out of the buffer.

        \return The first character in the buffer, or -1 if no data is
        available.
    **/
    int16_t Peek();

    /**
        \brief Wait until all outgoing data to the server has been sent.

        While the server is connected, blocks until the server has ACK'd all
        outgoing packets.
    **/
    void Flush();

    /**
        \brief Flush the received data.
    **/
    void FlushInput();

    /**
        \brief Close the client's connection to the server.
    **/
    void Close();

    /**
        \brief Send the buffer contents to the server.

        \param[in] buff A pointer to the beginning of the data to send.
        \param[in] size The maximum number of bytes to send.
        \return The number of bytes sent to the server.

        \note The contents of the supplied buffer represents the payload in an
        outgoing TCP packet.
    **/
    uint32_t Send(const uint8_t *buff, uint32_t size) override;

    /**
        \brief Send a TCP packet.

        Send a TCP packet to the remote with a single character as the payload.

        \param[in] charToSend A character to send to the remote.

        \return The number of bytes written.
    **/
    uint32_t Send(uint8_t charToSend) {
        return EthernetTcp::Send(charToSend);
    }

    /**
        \brief Send a TCP packet.

        Send a TCP packet to the remote with a string of characters as the
        payload.

        \param[in] nullTermStr A string to send to the remote.

        \return The number of bytes written.
    **/
    uint32_t Send(const char *nullTermStr) {
        return EthernetTcp::Send(nullTermStr);
    }

    /**
        \brief Returns the remote port of the server this client is connected to

        \return The server's remote port.
    **/
    uint16_t RemotePort();

    using EthernetTcp::LocalPort;

    /**
        \brief Returns the remote IP address of the server this client is
        connected to.

        \return The server's remote IP address.
    **/
    IpAddress RemoteIp();

    /**
        \brief Returns the connection timeout.

        \return The connection timeout, in milliseconds.
    **/
    uint16_t ConnectionTimeout() {
        return m_connectionTimeout;
    }

    /**
        \brief Set the connection timeout. This is the maximum amount of time
        to wait for a server to accept this client after establishing initial
        communication with the server.

        \param[in] timeout The new connection timeout value, in milliseconds.

        \note Constrained between #TCP_CONNECTION_TIMEOUT_MIN and
        #TCP_CONNECTION_TIMEOUT_MAX in milliseconds.
    **/
    void ConnectionTimeout(uint16_t timeout);

#ifndef HIDE_FROM_DOXYGEN
    virtual bool operator == (const EthernetTcpClient);
#endif // !HIDE_FROM_DOXYGEN

private:
    uint16_t m_connectionTimeout;
    bool m_dnsInitialized;

}; // EthernetTcpClient

} // ClearCore namespace

#endif // !__ETHERNETTCPCLIENT_H__