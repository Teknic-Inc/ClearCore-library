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
    \file EthernetTcp.h
**/

#ifndef __ETHERNETTCP_H__
#define __ETHERNETTCP_H__

#include "lwip/tcp.h"
#include <string.h>
#ifndef HIDE_FROM_DOXYGEN
namespace ClearCore {

/** The maximum number of allowable client connections at any given time. **/
#define CLIENT_MAX 8
/** The size of the buffer to hold incoming TCP data, in bytes. **/
#define TCP_DATA_BUFFER_SIZE 600

/**
    \brief A base class for an Ethernet TCP connection.

    This class is a basic interface for a TCP connection.

    For more detailed information on the ClearCore Ethernet system, check out
    the \ref EthernetMain informational page.
**/
class EthernetTcp {

public:
    /**
        ClearCore TCP connection state.
    **/
    typedef struct {
        struct tcp_pcb *pcb;    /*!< The LwIP PCB for the TCP connection. */
        uint16_t dataHead;      /*!< The head of the incoming data buffer. */
        uint16_t dataTail;      /*!< The tail of the incoming data buffer. */
        tcp_state state;        /*!< The state of this tcp_data. */
        uint8_t data[TCP_DATA_BUFFER_SIZE]; /*!< The incoming data buffer for
                                                        this TCP connection. */
    } TcpData;

    /**
        Construct a TCP connection with no existing TCP state information.
    **/
    EthernetTcp() : m_tcpData(nullptr) {};

    /**
        Construct a TCP connection with existing TCP state information.
    **/
    EthernetTcp(TcpData *tcpData);

    /**
        \brief Send a TCP packet.

        Send a TCP packet to the remote with a single character as the payload.

        \param[in] charToSend A character to send to the remote.

        \return The number of bytes written.
    **/
    uint32_t Send(uint8_t charToSend);

    /**
        \brief Send a TCP packet.

        Send a TCP packet to the remote with a string of characters as the
        payload.

        \param[in] nullTermStr A string to send to the remote.

        \return The number of bytes written.
    **/
    uint32_t Send(const char *nullTermStr) {
        return Send((const uint8_t *)nullTermStr, strlen(nullTermStr));
    }

    /**
        \brief Send a TCP packet.

        Send a TCP packet to the remote with a sequence of characters as the
        payload.

        \param[in] buff The data to send.
        \param[in] size The number of characters to send.

        \return The number of bytes written.
    **/
    virtual uint32_t Send(const uint8_t *buff, uint32_t size) = 0;

    /**
        \brief Get the local port number.

        \return Returns the local port number.
    **/
    uint16_t LocalPort();

    /**
        \brief Get the local port number.

        \return Returns the local port number.
    **/
    volatile const TcpData *ConnectionState() {
        return m_tcpData;
    }

protected:
    // The TCP state.
    TcpData *m_tcpData;

};  // EthernetTcp

#ifndef HIDE_FROM_DOXYGEN
/**
    \brief The TCP connection accepted callback.
    Allows a TCP server to accept clients.
**/
err_t TcpAccept(void *arg, struct tcp_pcb *newpcb, err_t err);

/**
    The TCP connection callback.
**/
err_t TcpConnect(void *arg, struct tcp_pcb *tpcb, err_t err);

/**
    The TCP connection error callback.
**/
void TcpError(void *arg, err_t err);

/**
    \brief Handles a packet received from a TCP connection.

    A TCP Receive callback function will be passed a NULL pbuf if the
    remote host has closed the connection. If we return ERR_OK or ERR_ABRT
    from this callback function we must have freed the pbuf, otherwise we
    must NOT have freed it.
 **/
err_t TcpReceive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                 err_t err);

/**
    The TCP send complete callback.
**/
err_t TcpSend(void *arg, struct tcp_pcb *tpcb, u16_t len);

/**
    Closes a TCP connection.
**/
void TcpClose(struct tcp_pcb *pcb, EthernetTcp::TcpData *data);
#endif // !HIDE_FROM_DOXYGEN

} // ClearCore namespace
#endif // !HIDE_FROM_DOXYGEN
#endif // !__ETHERNETTCP_H__

