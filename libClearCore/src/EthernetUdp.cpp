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

#include "EthernetUdp.h"
#include "EthernetManager.h"

namespace ClearCore {

extern EthernetManager &EthernetMgr;

EthernetUdp::EthernetUdp():
    m_udpData({}),
          m_udpLocalPort(0),
          m_outgoingPacket(nullptr),
          m_incomingPacket(nullptr),
          m_udpBytesAvailable(0),
          m_udpRemoteIpReceived(),
          m_udpRemotePortReceived(0),
          m_udpRemoteIpDestination(),
          m_udpRemotePortDestination(0),
          m_initialized(false),
          m_packetBegun(false),
          m_packetReadyToSend(false),
m_packetParsed(false) { }

bool EthernetUdp::Begin(uint16_t localPort) {
    if (m_initialized) {
        // Already initialized, nothing to do.
        return false;
    }

    m_udpLocalPort = localPort;

    // Set up the UDP state to pass to lwIP callbacks.
    m_udpData.pcb = udp_new();
    m_udpData.available = 0;
    m_udpBytesAvailable = 0;

    ip_addr_t ip = IPADDR4_INIT(uint32_t(EthernetMgr.LocalIp()));

    // Bind the UDP PCB to the local IP and port.
    err_t err = udp_bind(m_udpData.pcb, &ip, m_udpLocalPort);
    if (err != ERR_OK) {
        return false;
    }

    // Register the callback function upon receiving a UDP packet.
    udp_recv(m_udpData.pcb, UdpReceive, &m_udpData);

    m_packetBegun = false;
    m_packetReadyToSend = false;
    m_packetParsed = false;
    m_initialized = true;

    return m_initialized;
}

void EthernetUdp::End() {
    if (!m_initialized) {
        // Nothing to do.
        return;
    }

    if (m_udpData.pcb != nullptr) {
        // Remove the remote end of the PCB.
        udp_disconnect(m_udpData.pcb);
        // Remove PCB from the list of UDP PCBs and free the PCB.
        udp_remove(m_udpData.pcb);
        m_udpData.pcb = nullptr;
    }

    if (m_udpData.packet != nullptr) {
        // Free the packet. Since there may be several packets chained together,
        pbuf_free(m_udpData.packet);
    }

    m_udpData = {};

    if (m_incomingPacket != nullptr) {
        pbuf_free(m_incomingPacket);
        m_incomingPacket = nullptr;
    }

    if (m_outgoingPacket != nullptr) {
        pbuf_free(m_outgoingPacket);
        m_outgoingPacket = nullptr;
    }

    m_udpLocalPort = 0;
    m_udpRemoteIpReceived = IpAddress();
    m_udpRemotePortReceived = 0;
    m_udpBytesAvailable = 0;

    m_packetParsed = false;
    m_packetReadyToSend = false;
    m_packetBegun = false;
    m_initialized = false;
}

bool EthernetUdp::Connect(IpAddress ip, uint16_t port) {
    if (!m_initialized) {
        // Not initialized.
        return false;
    }
    EthernetMgr.Refresh();
    // Save the specified destination IP and port.
    m_udpRemoteIpDestination = ip;
    m_udpRemotePortDestination = port;

    m_packetBegun = true;
    m_packetReadyToSend = false;

    return m_packetBegun;
}

bool EthernetUdp::PacketSend() {
    if (!m_initialized || !m_packetBegun || !m_packetReadyToSend) {
        return false;
    }
    // Try to send the outgoing data.
    ip_addr_t destinationIp = IPADDR4_INIT(uint32_t(m_udpRemoteIpDestination));
    err_t err = udp_sendto(m_udpData.pcb, m_outgoingPacket,
                           &destinationIp, m_udpRemotePortDestination);

    // Free the outgoing packet buffer chain.
    pbuf_free(m_outgoingPacket);
    m_outgoingPacket = nullptr;
    m_udpRemoteIpDestination = IpAddress();
    m_udpRemotePortDestination = 0;
    m_packetBegun = false;
    m_packetReadyToSend = false;

    EthernetMgr.Refresh();

    // Report whether data was sent successfully.
    return err == ERR_OK;
}

uint32_t EthernetUdp::PacketWrite(uint8_t c) {
    return PacketWrite(&c, 1);
}

uint32_t EthernetUdp::PacketWrite(const char *nullTermStr) {
    uint32_t count = 0;
    while (*nullTermStr) {
        if (!PacketWrite(static_cast<uint8_t>(*nullTermStr++))) {
            break;
        }
        count++;
    }
    return count;
}

uint32_t EthernetUdp::PacketWrite(const uint8_t *buffer, uint32_t size) {
    err_t err;

    if (!m_initialized || !m_packetBegun) {
        // Not yet initialized or haven't called beginPacket() yet.
        return 0;
    }

    EthernetMgr.Refresh();

    if (m_outgoingPacket == nullptr) {
        // Allocate a new pbuf to hold the outgoing data.
        m_outgoingPacket = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);

        if (m_outgoingPacket == nullptr) {
            // Couldn't allocate an outgoing pbuf to hold data.
            return 0;
        }

        // Copy data from the supplied buffer into the outgoing pbuf data.
        err = pbuf_take(m_outgoingPacket, buffer, size);
        if (err != ERR_OK) {
            pbuf_free(m_outgoingPacket);
            // Couldn't copy data from buffer into outgoing pbuf.
            return 0;
        }
    }
    else {
        // The outgoing packet buffer already has some data in it that hasn't
        // been sent yet. Allocate a new pbuf with enough size to hold the old
        // data and the new data. After all the data is copied into the new pbuf
        // re-assign the outgoing pbuf to this one.
        struct pbuf *outgoing = pbuf_alloc(PBUF_TRANSPORT,
                                           size + m_outgoingPacket->tot_len,
                                           PBUF_RAM);
        if (outgoing == nullptr) {
            // Couldn't allocate a pbuf to hold all the data to write.
            return 0;
        }

        err = pbuf_copy(outgoing, m_outgoingPacket);
        if (err != ERR_OK) {
            pbuf_free(outgoing);
            // Couldn't copy the existing data into the outgoing pbuf.
            return 0;
        }

        err = pbuf_take_at(outgoing, buffer, size, m_outgoingPacket->tot_len);
        if (err != ERR_OK) {
            pbuf_free(outgoing);
            // Couldn't copy the new data into the outgoing pbuf.
            return 0;
        }

        // Replace the outgoing packet with the newly built packet.
        pbuf_free(m_outgoingPacket);
        m_outgoingPacket = outgoing;
    }

    m_packetReadyToSend = true;

    return m_outgoingPacket == nullptr ? 0 : size;
}

uint16_t EthernetUdp::PacketParse() {
    EthernetMgr.Refresh();
    if (!m_initialized || m_udpData.available == 0) {
        return 0;
    }

    // Save the state of the received packet.
    m_udpRemoteIpReceived = IpAddress(m_udpData.remoteIp.addr);
    m_udpRemotePortReceived = m_udpData.remotePort;
    m_udpBytesAvailable = m_udpData.available;

    // Make this packet available to read by storing a copy as a class member.
    // This prevents the received packet from being clobbered by subsequent
    // received packets while attempting to do a chain of read()s on the
    // original packet.
    if (m_incomingPacket != nullptr) {
        pbuf_free(m_incomingPacket);
        m_incomingPacket = nullptr;
    }

    m_incomingPacket = pbuf_alloc(PBUF_TRANSPORT, m_udpData.packet->tot_len,
                                  PBUF_RAM);
    if (m_incomingPacket == nullptr) {
        // Couldn't allocate a pbuf to copy the received packet into.
        return 0;
    }

    err_t err = pbuf_copy(m_incomingPacket, m_udpData.packet);
    if (err != ERR_OK) {
        // Copy failed.
        return 0;
    }

    // Free the received packet so we can receive another packet while reading
    // the one we copied.
    pbuf_free(m_udpData.packet);
    m_udpData.packet = nullptr;
    m_udpData.available = 0;

    m_packetParsed = true;

    return m_udpBytesAvailable;
}

uint16_t EthernetUdp::BytesAvailable() {
    EthernetMgr.Refresh();
    return m_udpBytesAvailable;
}

int32_t EthernetUdp::PacketRead(unsigned char *dataPtr, uint16_t length) {
    if (!m_initialized) {
        return -1;
    }

    if (!m_packetParsed || m_udpBytesAvailable == 0 || length == 0) {
        // No valid data packet received or request to read nothing.
        return -1;
    }

    // Limit read to what we have available.
    if (m_udpBytesAvailable < length) {
        length = m_udpBytesAvailable;
    }

    uint16_t bytesRead = UdpPacketRead(m_incomingPacket, &m_udpBytesAvailable,
                                       dataPtr, length);

    if (m_udpBytesAvailable == 0) {
        // We've read all the bytes of the received packet so free it.
        // Must call parsePacket() again before reading the next packet.
        pbuf_free(m_incomingPacket);
        m_incomingPacket = nullptr;
        m_packetParsed = false;
    }

    return bytesRead == 0 ? -1 : bytesRead;
}

int16_t EthernetUdp::Peek() {
    if (!m_initialized) {
        return -1;
    }

    if (!m_packetParsed || m_udpBytesAvailable == 0) {
        // No valid data packet received or request to read nothing.
        return -1;
    }

    return pbuf_get_at(m_incomingPacket, 0);
}

void EthernetUdp::PacketFlush() {
    if (!m_initialized || !m_packetParsed) {
        return;
    }

    pbuf_free(m_incomingPacket);
    m_incomingPacket = nullptr;
}

IpAddress EthernetUdp::RemoteIp() {
    return m_udpRemoteIpReceived;
}

uint16_t EthernetUdp::RemotePort() {
    return m_udpRemotePortReceived;
}

uint16_t EthernetUdp::UdpPacketRead(pbuf *packet, uint16_t *available,
                                    unsigned char *buffer, uint16_t size) {
    uint16_t bytesRead = 0;

    // Read from incoming packet data as long as we have a packet to read with
    // unread data available and there are requested bytes remaining to be read.
    while (packet != nullptr && *available > 0 && bytesRead < size) {
        // Calculate the location in the received packet to read from.
        uint16_t offset = packet->tot_len - *available;

        // Read byte by byte until there's nothing left to read:
        // either we've read all available bytes, read all bytes requested,
        // or we've exhausted the bytes available in the current pbuf and
        // need to move on to the next pbuf in the chain to continue reading.
        while ((*available > 0) && (bytesRead < size) &&
                (offset < packet->len)) {
            buffer[bytesRead++] = pbuf_get_at(packet, offset++);
            (*available)--;
        }
        // If we haven't read all the bytes requested, check if we can move
        // on to the next pbuf in the chain. If so, free the pbuf that had
        // all of its data read.
        if (bytesRead < size) {
            pbuf *pbufToFree = packet;
            packet = packet->next;

            if (packet != nullptr) {
                // Increase the ref count.
                pbuf_ref(packet);
            }
            // Free the removed packet
            pbuf_free(pbufToFree);
        }
    }
    return bytesRead;
}

/**
    lwIP UDP datagram received callback.
**/
void UdpReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                const ip_addr_t *addr, u16_t port) {
    EthernetUdp::UdpData *data = (EthernetUdp::UdpData *)arg;

    // Bail out if the udp_pcb received could not be allocated
    // or if we received to an unregistered udp_pcb.
    if (data == nullptr || data->pcb != pcb) {
        pbuf_free(p);
        return;
    }

    // Drop the last packet received.
    if (data->packet != nullptr) {
        pbuf_free(data->packet);
    }

    // Copy the remote IP, port, packet contents and number of bytes received
    // into the UDP state.
    ip_addr_copy(data->remoteIp, *addr);
    data->remotePort = port;
    data->packet = p;
    data->available = p->tot_len;
}

} // ClearCore namespace