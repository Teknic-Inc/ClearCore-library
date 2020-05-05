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

#include "EthernetTcpServer.h"
#include <stdlib.h>

namespace ClearCore {

extern EthernetManager &EthernetMgr;

EthernetTcpServer::EthernetTcpServer(uint16_t port)
    : EthernetTcp(), m_initialized(false), m_serverPort(port) {
    for (uint8_t i = 0; i < CLIENT_MAX; i++) {
        m_tcpDataClient[i] = nullptr;
    }
}

void EthernetTcpServer::Begin() {
    if (m_tcpData == nullptr) {
        // Allocate
        m_tcpData = static_cast<TcpData *>(calloc(1, sizeof(TcpData)));
    }
    if (m_tcpData->pcb != nullptr) {
        // Need to call stop() first. Server already in session.
        return;
    }

    m_tcpData->pcb = tcp_new();
    if (m_tcpData->pcb == nullptr) {
        // Couldn't allocate TCP PCB for server.
        return;
    }
    tcp_nagle_disable((m_tcpData->pcb));

    // Pass the server's TCP clients array to server TCP callbacks.
    tcp_arg(m_tcpData->pcb, &m_tcpDataClient);

    // Bind the PCB to the local IP address and port.
    ip_addr_t ip = IPADDR4_INIT(uint32_t(EthernetMgr.LocalIp()));

    err_t err = tcp_bind(m_tcpData->pcb, &ip, m_serverPort);
    if (err != ERR_OK) {
        // Could not bind TCP PCB to the port.
        TcpClose(m_tcpData->pcb, m_tcpData);
        return;
    }

    // Set the server's TCP connection state to LISTEN.
    m_tcpData->pcb = tcp_listen(m_tcpData->pcb);
    m_tcpData->state = LISTEN;

    // Register TCP connection accept callback.
    tcp_accept(m_tcpData->pcb, TcpAccept);

    m_initialized = true;
}

// EthernetServer manages the clients. A client is only identified and returned
// when data has been received from the client and is available for reading.
EthernetTcpClient EthernetTcpServer::Available() {
    EthernetMgr.Refresh();
    EthernetTcpClient client;

    for (uint8_t iClient = 0; iClient < CLIENT_MAX; iClient++) {
        TcpData *clientData = m_tcpDataClient[iClient];
        if (clientData == nullptr) {
            continue;
        }

        client = EthernetTcpClient(clientData);

        // Clean out stale/old/'disconnected' references.
        if (!client.Connected() && client.BytesAvailable() == 0) {
            free(clientData);
            m_tcpDataClient[iClient] = nullptr;
            continue;
        }

        // Return a client with available incoming data.
        if (client.BytesAvailable()) {
            return client;
        }
    }

    // No clients available. Return a default client with no TCP state.
    client = EthernetTcpClient();
    return client;
}

// EthernetServer gives a client only once, regardless of it it has sent data.
// Then, the user is responsible for keeping track of connected clients.
EthernetTcpClient EthernetTcpServer::Accept() {
    EthernetMgr.Refresh();
    EthernetTcpClient client;

    for (uint8_t iClient = 0; iClient < CLIENT_MAX; iClient++) {
        TcpData *clientData = m_tcpDataClient[iClient];
        // Skip null or empty references.
        if (clientData == nullptr) {
            continue;
        }

        client = EthernetTcpClient(clientData);

        // Clean out stale/old/'disconnected' references.
        if (!client.Connected()) {
            free(clientData);
            m_tcpDataClient[iClient] = nullptr;
            continue;
        }

        // Return any valid client.
        m_tcpDataClient[iClient] = nullptr;
        return client;
    }

    client = EthernetTcpClient();
    return client;
}

uint32_t EthernetTcpServer::Send(const uint8_t *buff, uint32_t size) {
    EthernetMgr.Refresh();

    for (uint8_t iClient = 0; iClient < CLIENT_MAX; iClient++) {
        TcpData *clientData = m_tcpDataClient[iClient];
        EthernetTcpClient client = EthernetTcpClient(clientData);
        // Don't send data to empty or closed connections.
        if (!client.Connected()) {
            continue;
        }
        if (clientData->state == ESTABLISHED) {
            client.Send(buff, size);
        }
    }
    return size;
}

bool EthernetTcpServer::Ready() {
    bool full = true;
    for (uint8_t iClient = 0; iClient < CLIENT_MAX; iClient++) {
        if (m_tcpDataClient[iClient] == nullptr) {
            full = false;
            break;
        }
    }
    return (m_initialized && !full);
}

} // ClearCore namespace