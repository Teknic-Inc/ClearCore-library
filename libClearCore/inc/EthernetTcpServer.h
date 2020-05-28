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
    \file EthernetTcpServer.h
**/

#ifndef __ETHERNETTCPSERVER_H__
#define __ETHERNETTCPSERVER_H__

#include "EthernetTcp.h"
#include "EthernetManager.h"
#include "EthernetTcpClient.h"

namespace ClearCore {

/**
    \brief ClearCore TCP server class.

    This class manages an instance of a TCP server and manages interactions
    with multiple Ethernet TCP client connections.

    For more detailed information on the ClearCore Ethernet system, check out
    the \ref EthernetMain informational page.
**/
class EthernetTcpServer : public EthernetTcp {

public:
    /**
        \brief Construct a TCP server.

        \param[in] port The local port to listen for incoming TCP connections
        on.
    **/
    explicit EthernetTcpServer(uint16_t port);

    /**
        \brief Set up the server to begin listening for incoming TCP connections
    **/
    void Begin();

    /**
        \brief Return a reference to a client that has incoming data.

        Returns a reference to a client that has incoming data available to
        read. The server will continue to manage the client.

        \return Returns a TCP client that has incoming data available to read.

        \note The server will continue to track the client's reference.
    **/
    EthernetTcpClient Available();

    /**
        \brief Return a client with an active connection.

        Returns a client with an active connection. The server will only return
        an active client once, and then the server will no longer manage the
        client's connection.

        \return Returns a TCP client with an active connection.

        \note The server will no longer track the client's reference.
    **/
    EthernetTcpClient Accept();

    /**
        \brief Send data to all clients managed by the server.

        Send a TCP packet to each client managed by the server with the
        contents of the provided buffer as the packet's payload.

        \param[in] buff A pointer to the beginning of the data to send.
        \param[in] size The maximum number of bytes to send.

        \return The number of bytes written to each client.
        \note The contents of the supplied buffer represents the payload in an
        outgoing TCP packet.
    **/
    uint32_t Send(const uint8_t *buff, uint32_t size) override;

    /**
        \brief Send data to all clients managed by the server.

        Send a TCP packet to each client managed by the server with a
        character as the payload.

        \param[in] charToSend A character to send to the clients.

        \return The number of bytes written to each client.
    **/
    uint32_t Send(uint8_t charToSend) {
        return EthernetTcp::Send(charToSend);
    }

    /**
        \brief Send data to all clients managed by the server.

        Send a TCP packet to each client managed by the server with a
        string of characters as the payload.

        \param[in] nullTermStr A string to send to the clients.

        \return The number of bytes written to each client.
    **/
    uint32_t Send(const char *nullTermStr) {
        return EthernetTcp::Send(nullTermStr);
    }

    using EthernetTcp::LocalPort;

    /**
        \brief Is the server ready to accept a client?

        \return Return true if the server is ready to accept clients.
    **/
    bool Ready();

private:
    bool m_initialized;

    // The server's listening port.
    uint16_t m_serverPort;

    // TCP state for connected clients.
    TcpData *m_tcpDataClient[CLIENT_MAX];

}; // EthernetTcpServer

} // ClearCore namespace

#endif // !__ETHERNETTCPSERVER_H__