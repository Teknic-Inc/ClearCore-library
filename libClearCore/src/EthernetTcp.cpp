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

#include "EthernetTcp.h"
#include "lwip/tcp.h"
#include <stdlib.h>

namespace ClearCore {

EthernetTcp::EthernetTcp(TcpData *tcpData)
    : m_tcpData(tcpData) {}

uint32_t EthernetTcp::Send(uint8_t charToSend) {
    return Send(&charToSend, 1);
}

uint16_t EthernetTcp::LocalPort() {
    if (m_tcpData == nullptr || m_tcpData->pcb == nullptr) {
        return 0;
    }
    return m_tcpData->pcb->local_port;
}

/**
    The TCP connection accepted callback.
    Allows a TCP server to accept clients.
**/
err_t TcpAccept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    EthernetTcp::TcpData **tcpClientData = (EthernetTcp::TcpData **)arg;

    // Set the priority for the new TCP connection. Make this higher than min?
    tcp_setprio(newpcb, TCP_PRIO_MIN);

    if ((tcpClientData == nullptr) || (err != ERR_OK)) {
        // Either the client PCBs could not be allocated or there was another
        // error that occurred during TCP connection setup.
        tcp_close(newpcb);
        return ERR_ARG;
    }

    EthernetTcp::TcpData *clientData =
        (EthernetTcp::TcpData *)calloc(1, sizeof(EthernetTcp::TcpData));
    if (clientData == nullptr) {
        TcpClose(newpcb, clientData);
        return ERR_MEM;
    }

    clientData->pcb = newpcb;
    clientData->state = ESTABLISHED;

    bool accepted = false;

    // Look for an open 'socket' to allow the client to connect to the server.
    for (uint8_t iClient = 0; iClient < CLIENT_MAX; iClient++) {
        if (tcpClientData[iClient] == nullptr) {
            tcpClientData[iClient] = clientData;
            accepted = true;
            break;
        }
    }

    if (!accepted) {
        TcpClose(newpcb, clientData);
        free(clientData);
        return ERR_MEM;
    }
    tcp_nagle_disable(newpcb);

    tcp_arg(newpcb, clientData);
    tcp_recv(newpcb, TcpReceive);
    tcp_err(newpcb, TcpError);
    tcp_sent(newpcb, TcpSend);

    return ERR_OK;
}

/**
    The TCP connection callback.
**/
err_t TcpConnect(void *arg, struct tcp_pcb *tpcb, err_t err) {
    EthernetTcp::TcpData *data = (EthernetTcp::TcpData *)arg;

    if (data == nullptr) {
        return ERR_ARG;
    }
    if (err != ERR_OK) {
        TcpClose(tpcb, data);
        return err;
    }
    if (data->pcb == nullptr || data->pcb != tpcb) {
        TcpClose(tpcb, data);
        return ERR_ARG;
    }

    data->state = ESTABLISHED;

    tcp_recv(tpcb, TcpReceive);
    tcp_sent(tpcb, TcpSend);
    tcp_err(tpcb, TcpError);

    return ERR_OK;
}

/**
    The TCP connection error callback.
**/
void TcpError(void *arg, err_t err) {
    EthernetTcp::TcpData *data = (EthernetTcp::TcpData *)arg;

    if ((data == nullptr) || (err == ERR_OK)) {
        return;
    }

    // At this point the TCP PCB is already free'd.
    data->pcb = nullptr;
    data->state = CLOSING;
}

/**
    Handles a packet received from a TCP connection.

    A TCP Receive callback function will be passed a NULL pbuf if the
    remote host has closed the connection. If we return ERR_OK or ERR_ABRT
    from this callback function we must have freed the pbuf, otherwise we
    must NOT have freed it.
**/
err_t TcpReceive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    EthernetTcp::TcpData *tcpClientData =
        static_cast<EthernetTcp::TcpData *>(arg);
    if (tcpClientData == nullptr) {
        return ERR_ARG;
    }
    // A NULL pbuf indicates that the remote host closed the connection.
    if (p == NULL) {
        TcpClose(tpcb, tcpClientData);
        return ERR_OK;
    }
    // If return anything other than ERR_OK or ERR_ABRT, must NOT free pbuf.
    if (err != ERR_OK) {
        return err;
    }
    if (tcpClientData->state == ESTABLISHED) {
        // Only copy the packet's payload if we have enough empty space to copy
        // every byte.
        int32_t availableToRead =
            tcpClientData->dataTail - tcpClientData->dataHead;
        if (availableToRead  < 0) {
            availableToRead += TCP_DATA_BUFFER_SIZE;
        }
        if (TCP_DATA_BUFFER_SIZE - availableToRead < p->tot_len) {
            return ERR_BUF;
        }
        // Put received packet into the buffer.
        uint16_t bytesReceived = p->tot_len;
        for (uint16_t i = 0; i < bytesReceived; i++) {
            uint16_t nextIndex = (tcpClientData->dataTail + 1) %
                                 TCP_DATA_BUFFER_SIZE;
            // If the buffer is full drop the rest
            if (nextIndex == tcpClientData->dataHead) {
                bytesReceived = i;
                break;
            }
            // If the buffer is not full, save the value
            tcpClientData->data[tcpClientData->dataTail] = pbuf_get_at(p, i);
            tcpClientData->dataTail = nextIndex;
        }
        // Acknowledge the data was received.
        tcp_recved(tcpClientData->pcb, bytesReceived);
        // Must free the pbuf
        pbuf_free(p);
        return ERR_OK;
    }
    // If we got this far, we've received a non-empty frame without error,
    // but there's no established TCP connection.
    tcp_recved(tcpClientData->pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

/**
    The TCP send complete callback.
**/
err_t TcpSend(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    (void)len;
    EthernetTcp::TcpData *data = (EthernetTcp::TcpData *)arg;

    // Either the TCP PCB couldn't be allocated or the PCB target is wrong.
    if (data == nullptr || data->pcb != tpcb) {
        return ERR_ARG;
    }
    return ERR_OK;
}

/**
    Closes a TCP connection.
**/
void TcpClose(struct tcp_pcb *pcb, EthernetTcp::TcpData *data) {
    // Remove all the callbacks.
    tcp_accept(pcb, nullptr);
    tcp_err(pcb, nullptr);
    tcp_poll(pcb, nullptr, 0);
    tcp_recv(pcb, nullptr);
    tcp_sent(pcb, nullptr);

    // Close the TCP connection and free the PCB.
    tcp_close(pcb);

    // Clean up the TCP data.
    if (data != nullptr) {
        data->pcb = nullptr;
        data->state = CLOSING;
    }
}

} // ClearCore namespace