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

#include "EthernetTcpClient.h"

#include "EthernetManager.h"
#include "EthernetTcp.h"
#include "SysTiming.h"
#include "SysUtils.h"
#include <stdlib.h>

namespace ClearCore {

extern EthernetManager &EthernetMgr;

EthernetTcpClient::EthernetTcpClient()
    : EthernetTcp(),
      m_connectionTimeout(2000),
      m_dnsInitialized(false) {}

EthernetTcpClient::EthernetTcpClient(TcpData *tcpData)
    : EthernetTcp(tcpData),
      m_connectionTimeout(2000),
      m_dnsInitialized(false) {}

bool EthernetTcpClient::Connect(IpAddress ip, uint16_t port) {
    if (m_tcpData != nullptr && Connected()) {
        // Already connected.
        return false;
    }
    if (m_tcpData == nullptr) {
        m_tcpData = static_cast<TcpData *>(calloc(1, sizeof(TcpData)));
        if (m_tcpData == nullptr) {
            // Couldn't allocate TCP state.
            return false;
        }
    }

    m_tcpData->pcb = tcp_new();
    if (m_tcpData->pcb == nullptr) {
        free(m_tcpData);
        // Couldn't allocate TCP PCB.
        return false;
    }
    tcp_nagle_disable(m_tcpData->pcb);

    // Pass the TCP state to TCP callbacks.
    tcp_arg(m_tcpData->pcb, m_tcpData);

    m_tcpData->state = CLOSED;

    ip_addr_t ipaddr = IPADDR4_INIT(uint32_t(ip));
    err_t err = tcp_connect(m_tcpData->pcb, &ipaddr, port, TcpConnect);
    if (err != ERR_OK) {
        Close();
        return false;
    }

    uint32_t start = Milliseconds();
    while (m_tcpData->state == CLOSED) {
        EthernetMgr.Refresh();
        uint32_t elapsed = Milliseconds() - start;
        // Timeout for connection established.
        if ((elapsed > m_connectionTimeout) || (m_tcpData->state == CLOSING)) {
            Close();
            free(m_tcpData);
            m_tcpData = nullptr;
            return false;
        }
    }
    return true;
}

bool EthernetTcpClient::Connected() {
    if (m_tcpData == nullptr || m_tcpData->pcb == nullptr) {
        return false;
    }
    tcp_state tcpState = m_tcpData->pcb->state;
    return (tcpState != CLOSING && tcpState != CLOSE_WAIT &&
            tcpState != CLOSED);
}

int16_t EthernetTcpClient::BytesAvailable() {
    EthernetMgr.Refresh();
    if (m_tcpData == nullptr) {
        return 0;
    }
    // Calculate the available data in the buffer
    int32_t difference = m_tcpData->dataTail - m_tcpData->dataHead;

    return (difference < 0) ? TCP_DATA_BUFFER_SIZE + difference : difference;
}

int16_t EthernetTcpClient::Read() {
    uint8_t buff;
    int16_t bytesRead = Read(&buff, 1);
    return bytesRead == 1 ? buff : -1;
}

int16_t EthernetTcpClient::Read(uint8_t *dataPtr, uint32_t length) {
    if (m_tcpData == nullptr) {
        // Not initialized.
        return -1;
    }
    uint16_t bytesRead = 0;
    // Read from the TCP's incoming data buffer.
    while (m_tcpData->dataTail != m_tcpData->dataHead && bytesRead < length) {
        uint16_t nextIndex = (m_tcpData->dataHead + 1) % TCP_DATA_BUFFER_SIZE;
        dataPtr[bytesRead++] = m_tcpData->data[m_tcpData->dataHead];
        m_tcpData->dataHead = nextIndex;
    }
    return bytesRead;
}

int16_t EthernetTcpClient::Peek() {
    if (m_tcpData == nullptr || m_tcpData->dataTail == m_tcpData->dataHead) {
        // Not initialized or no data to read.
        return -1;
    }
    int16_t peekChar = m_tcpData->data[m_tcpData->dataHead];
    return peekChar;
}

void EthernetTcpClient::Flush() {
    if (m_tcpData == nullptr || m_tcpData->pcb == nullptr) {
        // Not initialized or no connection.
        return;
    }
    while (Connected()) {
        // All outgoing data has been sent when there is nothing unsent and
        // nothing unacked.
        if (m_tcpData->pcb->unsent == nullptr &&
                m_tcpData->pcb->unacked == nullptr) {
            break;
        }
        EthernetMgr.Refresh();
    }
}

void EthernetTcpClient::FlushInput() {
    if (m_tcpData == nullptr) {
        return;
    }
    m_tcpData->dataHead = 0;
    m_tcpData->dataTail = 0;
}

void EthernetTcpClient::Close() {
    if (m_tcpData == nullptr) {
        // No connection, nothing to do.
        return;
    }
    // Close the TCP connection with FIN.
    if (m_tcpData->state != CLOSING) {
        TcpClose(m_tcpData->pcb, m_tcpData);
    }
    free(m_tcpData);
    m_tcpData = nullptr;
}

uint32_t EthernetTcpClient::Send(const uint8_t *buffer, uint32_t size) {
    if (m_tcpData == nullptr || m_tcpData->pcb == nullptr ||
            size == 0 || buffer == nullptr) {
        // State hasn't been initialized or requested zero bytes.
        return 0;
    }

    if (m_tcpData->state != ESTABLISHED) {
        // The connection has not yet been established.
        return 0;
    }

    // Check the # of bytes available in the TCP send buffer.
    uint32_t bufferAvailable = tcp_sndbuf(m_tcpData->pcb);
    uint32_t bytesToWrite = min(bufferAvailable, size);
    err_t err = tcp_write(m_tcpData->pcb, buffer, bytesToWrite,
                          TCP_WRITE_FLAG_COPY);

    if (err != ERR_OK) {
        return 0;
    }
    // Initiate output immediately.
    err = tcp_output(m_tcpData->pcb);
    if (err != ERR_OK) {
        return 0;
    }
    // Avoid filling the send queue.
    uint32_t TCP_SEND_TIMEOUT_MS = 5;
    uint32_t startMs = Milliseconds();
    while (m_tcpData->pcb->snd_queuelen >= TCP_SND_QUEUELEN >> 1) {
        if (Milliseconds() - startMs >= TCP_SEND_TIMEOUT_MS) {
            break;
        }
        EthernetMgr.Refresh();
    }
    return bytesToWrite;
}

uint16_t EthernetTcpClient::RemotePort() {
    if (m_tcpData == nullptr || m_tcpData->pcb == nullptr) {
        return 0;
    }
    return m_tcpData->pcb->remote_port;
}

IpAddress EthernetTcpClient::RemoteIp() {
    if (m_tcpData == nullptr || m_tcpData->pcb == nullptr) {
        return IpAddress();
    }
    return IpAddress(m_tcpData->pcb->remote_ip.addr);
}

void EthernetTcpClient::ConnectionTimeout(uint16_t timeout) {
    if (timeout <= TCP_CONNECTION_TIMEOUT_MIN) {
        m_connectionTimeout = TCP_CONNECTION_TIMEOUT_MIN;
    }
    else if (timeout >= TCP_CONNECTION_TIMEOUT_MAX) {
        m_connectionTimeout = TCP_CONNECTION_TIMEOUT_MAX;
    }
    else {
        m_connectionTimeout = timeout;
    }
}

bool EthernetTcpClient::operator ==(const EthernetTcpClient tcpClient) {
    return (m_tcpData != nullptr) && (m_tcpData == tcpClient.m_tcpData) &&
           (m_tcpData->pcb == tcpClient.m_tcpData->pcb);
}

} // ClearCore namespace