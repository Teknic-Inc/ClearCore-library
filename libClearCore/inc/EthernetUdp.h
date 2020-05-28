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

/*
    \file EthernetUdp.h
**/

#ifndef __ETHERNETUDP_H__
#define __ETHERNETUDP_H__

#include "IpAddress.h"
#include "lwip/udp.h"

namespace ClearCore {

/**
    \brief ClearCore UDP session class.

    This class manages a single local UDP session.

    For more detailed information on the ClearCore Ethernet system, check out
    the \ref EthernetMain informational page.
**/
class EthernetUdp {

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        ClearCore UDP connection state.
    **/
    typedef struct {
        struct udp_pcb *pcb; /*!< The LwIP PCB for the UDP connection. */
        struct pbuf *packet; /*!< The incoming data buffer. */
        uint16_t available;  /*!< The number of available incoming bytes. */
        ip_addr_t remoteIp;  /*!< The remote IP address of the incoming data. */
        u16_t remotePort;    /*!< The remote port of the incoming data. */
    } UdpData;
#endif // !HIDE_FROM_DOXYGEN

    /**
        Construct an Ethernet UDP session.
    **/
    EthernetUdp();

    /**
        \brief  Initialize the UDP session and begin listening on the specified
        local port.

        \param[in] localPort The local port for the UDP session.

        \return success
    **/
    bool Begin(uint16_t localPort);

    /**
        \brief Disable the UDP session.
    **/
    void End();

    /**
        \brief Setup to send a UDP packet to a specified remote.

        \param[in] remoteIp The remote IP address.
        \param[in] remotePort The remote port.

        \return success
    **/
    bool Connect(IpAddress remoteIp, uint16_t remotePort);

    /**
        \brief Send the UDP packet set up with Connect().

        \return success
    **/
    bool PacketSend();

    /**
        \brief Write data into the outgoing UDP packet.

        Write data into the outgoing UDP packet set up with Connect().

        \param[in] c Character to write.

        \return The number of bytes written into the outgoing UDP packet.
    **/
    uint32_t PacketWrite(uint8_t c);

    /**
        \brief Write data into the outgoing UDP packet.

        Write a string of characters to the UDP packet set up with Connect().

        \param[in] nullTermStr The string to be sent.

        \return The number of bytes written into the outgoing UDP packet.
    **/
    uint32_t PacketWrite(const char *nullTermStr);

    /**
        \brief PacketWrite data into the outgoing UDP packet.

        Write data into the outgoing UDP packet set up with Connect().

        \param[in] buffer A pointer to the beginning of the data to write.
        \param[in] size The maximum number of bytes to write.

        \return The number of bytes written into the outgoing UDP packet.
    **/
    uint32_t PacketWrite(const uint8_t *buffer, uint32_t size);

    /**
        \brief Check for the newest incoming UDP packet.

        Checks for the newest incoming UDP packet received by the listening
        UDP session. Moves the packet to be read from by following calls to
        PacketRead().

        \return The size of the incoming packet in bytes.
    **/
    uint16_t PacketParse();

    /**
        \brief Number of bytes available to read from the current packet.

        \note PacketParse() must be called first to read an incoming packet.
    **/
    uint16_t BytesAvailable();

    /**
        \brief Reads the current packet received from the UDP session.

        \param[out] dataPtr Where to write the data to.
        \param[in] len Maximum number of bytes to write into dataPtr.

        \return The number of bytes read.

        \note PacketParse() must be called first to read an incoming packet.
    **/
    int32_t PacketRead(unsigned char *dataPtr, uint16_t len);

    /**
        \brief Attempts to get the next available character.

        Attempts to get the next available character without pulling the
        character out of the incoming packet.

        \return The first character in the incoming packet, or -1 if no data is
        available.

        \note PacketParse() must be called first to read an incoming packet.
    **/
    int16_t Peek();

    /**
        \brief Flush the current packet.
    **/
    void PacketFlush();

    /**
        \brief Returns the remote IP address for the current packet.

        \return The remote IP address.
    **/
    IpAddress RemoteIp();

    /**
        \brief Returns the remote port for the current packet.

        \return The remote port.
    **/
    uint16_t RemotePort();

private:
    UdpData m_udpData;
    uint16_t m_udpLocalPort;

    struct pbuf *m_outgoingPacket;

    struct pbuf *m_incomingPacket;
    uint16_t m_udpBytesAvailable;
    IpAddress m_udpRemoteIpReceived;
    uint16_t m_udpRemotePortReceived;
    IpAddress m_udpRemoteIpDestination;
    uint16_t m_udpRemotePortDestination;

    // Begin() was called.
    bool m_initialized;
    // Connect() was called and we can write to a packet.
    bool m_packetBegun;
    // PacketWrite() was called and we can send a packet.
    bool m_packetReadyToSend;
    // PacketParse() was called and we can read a packet.
    bool m_packetParsed;

    uint16_t UdpPacketRead(pbuf *packet, uint16_t *available,
                           unsigned char *buffer, uint16_t size);
}; // EthernetUdp

#ifndef HIDE_FROM_DOXYGEN

void UdpReceive(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                const ip_addr_t *addr, u16_t port);

#endif // !HIDE_FROM_DOXYGEN
} // ClearCore namespace
#endif // !__ETHERNETUDP_H__