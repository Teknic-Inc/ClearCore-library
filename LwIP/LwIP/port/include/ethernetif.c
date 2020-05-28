
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
    \file ethernetif.c
    \brief A device driver for the ClearCore to use LwIP.
**/
#include "EthernetApi.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#include <string.h>

#include "lwip/def.h"
#if LWIP_IPV6
#include "lwip/ethip6.h"
#endif
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/opt.h"
#include "lwip/pbuf.h"
#if LWIP_SNMP
#include "lwip/snmp.h"
#endif
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "sam.h"

typedef struct netif netInt;
typedef struct pbuf packetBuf;

/**
    \brief Determine the total length of a received packet.

    \param ethernetif   An Ethernet interface reference structure.
    \return The size of the packet in bytes.
**/
uint32_t PacketLength(ethInt *ethernetif) {
    // Calculated length of the received packet
    uint32_t length = 0;

    uint8_t sf = 0;
    uint32_t index;

    // Start at the current RX buffer index.
    index = *(ethernetif->rxBuffIndex);

    // Find the length of the packet.
    for (uint32_t i = 0; i < RX_BUFF_CNT; i++) {
        // The OWN bit indicates software has ownership of this buffer.
        if (!ethernetif->rxDesc[index].bit.OWN) {
            break;
        }
        // Check for the beginning of the frame.
        if (ethernetif->rxDesc[index].bit.SF) {
            sf = 1;
        }
        // If the beginning of the frame has been found, sum the length.
        if (sf == 1) {
            length += ethernetif->rxDesc[index].bit.LEN;
        }
        // Check for the end of the frame.
        if (ethernetif->rxDesc[index].bit.EF) {
            break;
        }
        // Increment the local index, treating RX buffers as circular.
        index = (index + 1) % RX_BUFF_CNT;
    }
    return length;
}

/**
    \brief Copies a frame into a buffer for a packet to be built.

    \param ethernetif   An Ethernet interface reference structure.
    \param buffer       The destination buffer for the contents of the frame.
    \param bytesToCopy  The total number of bytes that may be copied into buffer.

    \return The total number of bytes copied.
**/
uint32_t PacketRead(ethInt *ethernetif, uint8_t *buffer, uint32_t bytesToCopy) {
    // Offsets to the SF and EF RX buffers from the current RX index.
    uint8_t startFrameOffset = RX_BUFF_CNT;
    uint8_t endFrameOffset = RX_BUFF_CNT;
    // Count of RX buffers in the frame.
    uint8_t bufferCount = 0;
    // Total length of the packet.
    uint32_t packetLength;
    // Start at the current RX index.
    uint8_t index = *(ethernetif->rxBuffIndex);

    // Determine the number of buffers in the frame.
    for (uint8_t i = 0; i < RX_BUFF_CNT; i++) {
        // The OWN bit indicates software has ownership of this buffer.
        if (!ethernetif->rxDesc[index].bit.OWN) {
            break;
        }
        // The SF bit indicates this RX buffer is the first in the frame.
        if (ethernetif->rxDesc[index].bit.SF) {
            startFrameOffset = i;
        }
        // THE EF bit indicates this RX buffer is the last in the frame.
        if (ethernetif->rxDesc[index].bit.EF && startFrameOffset != RX_BUFF_CNT) {
            endFrameOffset = i;
            packetLength = ethernetif->rxDesc[index].bit.LEN;
            bytesToCopy = min(packetLength, bytesToCopy);
            break;
        }
        // Increment the local index, treating RX buffers as circular.
        index = (index + 1) % RX_BUFF_CNT;
    }

    if (endFrameOffset == RX_BUFF_CNT || startFrameOffset == RX_BUFF_CNT) {
        return 0; // Failed to find the frame..
    }

    // Bytes moved into buffer.
    uint32_t bytesCopied = 0;

    // Move the RX index to be at the start of frame RX buffer.
    *ethernetif->rxBuffIndex = (*(ethernetif->rxBuffIndex) + startFrameOffset) % RX_BUFF_CNT;
    // Determine the number of buffers in the frame.
    bufferCount = endFrameOffset - startFrameOffset + 1;
    if (endFrameOffset < startFrameOffset) {
        bufferCount += RX_BUFF_CNT;
    }
    // Copy the RX buffer(s) contents into buffer, treating RX buffers as circular.
    for (uint8_t i = 0; i < bufferCount; i++) {
        if (bytesToCopy > 0) {
            uint32_t bytes = min(bytesToCopy, RX_BUFFER_SIZE);
            // Mask to ignore the lowest 2 bits that are not part of the address.
            memcpy(buffer, (void *)(ethernetif->rxDesc[*(ethernetif->rxBuffIndex)].reg[0] & 0xFFFFFFFC), bytes);
            buffer += bytes;
            bytesCopied += bytes;
            bytesToCopy -= bytes;
            // Give ownership of this buffer back to hardware.
        }
        ethernetif->rxDesc[*(ethernetif->rxBuffIndex)].bit.OWN = 0;
        // Increment the buffer index, treating RX buffers as circular.
        *ethernetif->rxBuffIndex = (*(ethernetif->rxBuffIndex) + 1) % RX_BUFF_CNT;
    }
    return bytesCopied;
}

static err_t PacketWrite(ethInt *ethernetif, uint8_t *buffer, uint32_t length) {
    uint16_t startIndex = *ethernetif->txBuffIndex;
    uint16_t endIndex = *ethernetif->txBuffIndex;

    // Check that enough TX buffers are available to fit the entire frame.
    // Start at the current TX index.
    uint8_t index = *ethernetif->txBuffIndex;
    for (uint16_t i = 0; i < TX_BUFF_CNT; i++) {
        uint8_t tempIndex = (index + i) % TX_BUFF_CNT;
        // LwIP recommends just waiting for something to be available..
        while (ethernetif->txDesc[tempIndex].bit.OWN != 1) {
            continue;
        }
        // GMAC only returns the first TX buffer descriptor to ownership
        // on transmission complete. Reclaim remaining TX buffers.
        uint8_t buffLb;
        do {
            buffLb = ethernetif->txDesc[tempIndex].bit.LB;
            // Reclaim TX buffers that should belong to software.
            ethernetif->txDesc[tempIndex].bit.LB = 1;
            ethernetif->txDesc[tempIndex].bit.OWN = 1;
            tempIndex = (tempIndex + 1) % TX_BUFF_CNT;
        } while (buffLb == 0);
        // Require that one additional buffer always remain available/empty.
        if (length < TX_BUFFER_SIZE * i) {
            break;
        }
    }

    // Write into the transmit buffer(s).
    for (uint32_t i = 0; i < TX_BUFF_CNT; i++) {
        uint32_t bufferLength = min(length, TX_BUFFER_SIZE);
        memcpy((void *)(ethernetif->txDesc[*ethernetif->txBuffIndex].reg[0]),
               buffer + (i * TX_BUFFER_SIZE), bufferLength);
        length -= bufferLength;

        // Clear all fields except OWN or WRAP.
        ethernetif->txDesc[*ethernetif->txBuffIndex].reg[1] &= (0xC0000000);
        // Set only LEN.
        ethernetif->txDesc[*ethernetif->txBuffIndex].bit.LEN = bufferLength;

        if (length <= 0) {
            // Indicate last buffer of this frame.
            ethernetif->txDesc[*ethernetif->txBuffIndex].bit.LB = 1;
            endIndex = *ethernetif->txBuffIndex;
        }

        // Increment the TX buffer index.
        *ethernetif->txBuffIndex = (*ethernetif->txBuffIndex + 1) % TX_BUFF_CNT;

        if (length <= 0) {
            break;
        }
    }

    // Pass the transmit buffers for this frame to the GMAC.
    for (uint32_t i = endIndex; i != startIndex; i = (i + TX_BUFF_CNT - 1) % TX_BUFF_CNT) {
        ethernetif->txDesc[i].bit.OWN = 0;
    }
    // Final hand-off to the GMAC.
    ethernetif->txDesc[startIndex].bit.OWN = 0;

    // Activate the transmit.
    GMAC->NCR.bit.TSTART = 1;

    return ERR_OK;
}

/**
    In this function, the hardware should be initialized.
    Called from ethernetif_init().

    @param netif the already initialized lwip network interface structure
        for this ethernetif
**/
static void low_level_init(netInt *netif) {
    // Setup MAC address & TIDM
    // Specific Address Bottom stores the first four bytes.
    memcpy((void *)&GMAC->Sa[0].SAB.reg, netif->hwaddr, 4);
    // Specific Address Top stores the last two bytes.
    memcpy((void *)&GMAC->Sa[0].SAT.reg, netif->hwaddr + 4, 2);

#if LWIP_IPV6 && LWIP_IPV6_MLD
  // May need to implement something here if we are using MAC filtering.
#endif

}

/**
    This function should do the actual transmission of the packet. The packet is
    contained in the pbuf that is passed to the function. This pbuf
    might be chained.

    @param netif the lwip network interface structure for this ethernetif
    @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
    @return ERR_OK if the packet could be sent
        an err_t value if the packet couldn't be sent

    @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
        strange results. You might consider waiting for space in the DMA queue
        to become available since the stack doesn't retry to send a packet
        dropped because of memory failure (except for the TCP timers).
**/
static err_t low_level_output(netInt *netif, packetBuf *p) {
    ethInt *ethernetif;
    packetBuf *q;
    void * tempBuffer;
    uint8_t * index;
    err_t err;
    ethernetif = (ethInt *)(netif->state);

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); // Drop the padding word.
#endif

    if (p->tot_len == p->len) {
        err = PacketWrite(ethernetif, (uint8_t *)p->payload, p->tot_len);
    }
    else {
        tempBuffer = mem_malloc(LWIP_MEM_ALIGN_SIZE(p->tot_len));
        index = (uint8_t *)tempBuffer;
        if (tempBuffer == NULL) {
            return ERR_MEM; // Allocation error.
        }
        for (q = p; q != NULL; q = q->next) {
            memcpy(index, q->payload, q->len);
            index += q->len;
        }
        err = PacketWrite(ethernetif, (uint8_t *)tempBuffer, p->tot_len);
        mem_free(tempBuffer);
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); // Reclaim the padding word.
#endif

    LINK_STATS_INC(link.xmit);
    return err;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
/**
    Should allocate a pbuf and transfer the bytes of the incoming
    packet from the interface into the pbuf.

    @param netif the lwip network interface structure for this ethernetif
    @return a pbuf filled with the received packet (including MAC header)
        NULL on memory error
**/
static packetBuf *low_level_input(netInt *netif) {
    ethInt *ethernetif;
    packetBuf *p;
    uint32_t length;
    ethernetif = (ethInt *)netif->state;

    // Obtain the size of the packet.
    length = PacketLength(ethernetif);

    if (length == 0) {
        return NULL;
    }

    // Allow room for Ethernet padding.
#if ETH_PAD_SIZE
    length += ETH_PAD_SIZE;
#endif

    // Allocate a packet buffer.
    p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);

    if (p != NULL) {
#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); // Drop the padding word.
#endif
        // read the packet into the buffer
        PacketRead(ethernetif, (uint8_t*)p->payload, p->len);

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); // Reclaim the padding word.
#endif

        LINK_STATS_INC(link.recv);
    } 
    else { // P == NULL
        PacketRead(ethernetif, NULL, 0);
        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }
    return p;
}

/**
    This function should be called when a packet is ready to be read
    from the interface. Then the type of the received packet is determined and
    the appropriate input function is called.

    @param netif the lwip network interface structure for this ethernetif
**/
static void ethernetif_input(netInt *netif, packetBuf *p) {
    u16_t packetType;
    // Determine packet type from payload's Ethernet header.
    packetType = htons(((struct eth_hdr *)p->payload)->type);
    switch (packetType) {
        case ETHTYPE_ARP:   // Address resolution protocol
        // fall through
        case ETHTYPE_IP:    //  Internet protocol v4
            if (netif->input(p, netif) == ERR_OK) {
                break;
            }
            // Otherwise fall through to default of freeing pbuf.
            LWIP_DEBUGF(NETIF_DEBUG, ("IP input error.."));
            // fall through
        default:
            pbuf_free(p);
            p = NULL;
            break;
    }
}
#pragma GCC diagnostic push

/**
    Should be called at the beginning of the program to set up the
    network interface. It calls the function low_level_init() to do the
    actual setup of the hardware.

    This function should be passed as a parameter to netif_add().

    @param netif the lwip network interface structure for this ethernetif
    @return ERR_OK if the loopif is initialized
        ERR_MEM if private data couldn't be allocated
        any other err_t on error
 */
err_t ethernetif_init(netInt *netif) {
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    // flags to set device capabilities
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                   NETIF_FLAG_ETHERNET;
    // maximum transfer unit
    netif->mtu = 1536;

    // MAC address
    ethInt* ethernetif;

    ethernetif = (ethInt *) netif->state;
    netif->hwaddr_len = NETIF_MAX_HWADDR_LEN;
    memcpy(netif->hwaddr, ethernetif->mac, NETIF_MAX_HWADDR_LEN);

    // interface hostname (?)

    // descriptive name (only allows len 2)
    netif->name[0] = 'T';
    netif->name[1] = 'C';

    low_level_init(netif);

    return ERR_OK;

}