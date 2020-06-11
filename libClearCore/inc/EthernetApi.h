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

#ifndef HIDE_FROM_DOXYGEN
/**
    \file EthernetApi.h

    This file contains standard definitions for the receive (RX) and transmit
    (TX) descriptors as defined by the datasheet.
    This file contains a definition for a local interface struct to be
    used in conjunction with LwIP.
**/

#ifndef __ETHERNETAPI_H__
#define __ETHERNETAPI_H__

#include <stdint.h>

#ifndef TX_BUFF_CNT
#define TX_BUFF_CNT (8)
#endif

#ifndef RX_BUFF_CNT
#define RX_BUFF_CNT (16)
#endif

#ifndef TX_BUFFER_SIZE
#define TX_BUFFER_SIZE (520)
#endif

#ifndef RX_BUFFER_SIZE
#define RX_BUFFER_SIZE (128)
#endif

/**
    \brief Ethernet receive buffer descriptor.

    A receive buffer descriptor list entry as described by the data sheet.
**/
typedef union {
    struct {
uint32_t OWN :
        1;     /*!< bit:  0       Ownership of this buffer.                 */
uint32_t WRAP :
        1;    /*!< bit:  1       Last descriptor in receive buffer list    */
uint32_t ADDR :
        30;   /*!< bit:  2..31   Address of beginning of buffer            */
uint32_t LEN :
        13;    /*!< bit:  0..12   Length of the received frame              */
uint32_t FCS :
        1;     /*!< bit:  13      Meaning depending on jumbo frames and ignore FCS */
uint32_t SF :
        1;      /*!< bit:  14      Start of Frame                           */
        uint32_t EF : 1;      /*!< bit:  15      End of Frame   */
        uint32_t CFI : 1;     /*!< bit:  16      Canonical Format Indicator   */
        uint32_t VLAN : 3;    /*!< bit:  17..19  VLAN priority  */
        uint32_t PTAG : 1;    /*!< bit:  20      Priority Tag detected */
        uint32_t VTAG : 1;    /*!< bit:  21      VLAN Tag detected */
uint32_t CSM :
        2;     /*!< bit:  22..23  Meaning depends on whether RX checksum offloading enabled */
uint32_t SNAP :
        1;    /*!< bit   24      Meaning depends on whether RX checksum offloading enabled */
uint32_t SPAMI :
        2;   /*!< bit   25..26  Specific Address Register Match -- which address */
uint32_t SPAM :
        1;    /*!< bit   27      Specific Address Register Match found */
        uint32_t: 1;          /*!< bit   28      Reserved */
        uint32_t UHM : 1;     /*!< bit   29      Unicast Hash Match */
        uint32_t MHM : 1;     /*!< bit   30      Multicast Hash Match */
        uint32_t GAO : 1;     /*!< bit   31      Global All Ones Broadcast Address */
    } bit;
    uint32_t reg[2];
} GMAC_RX_DESC;

/**
   \brief Ethernet transmit buffer descriptor.

   A transmit buffer descriptor list entry as described by the data sheet.
**/
typedef union {
    struct {
        uint32_t ADDR : 32;   /*!< bit   0..31   Byte Address of Buffer */
        uint32_t LEN : 14;    /*!< bit   0..13   Length of buffer */
        uint32_t: 1;          /*!< bit   14      Reserved */
        uint32_t LB : 1;      /*!< bit   15      Last Buffer of the current frame */
uint32_t CRC :
        1;     /*!< bit   16      Data in buffer already contains a valid CRC */
        uint32_t: 3;          /*!< bit   17..19  Reserved */
uint32_t CSER :
        3;    /*!< bit   20..22  Transmit IP/TCP/UDP checksum generation offload errors */
        uint32_t: 3;          /*!< bit   23..25  Reserved */
uint32_t LCERR :
        1;   /*!< bit   26      Late collision, transmit error detected */
uint32_t FCERR :
        1;   /*!< bit   27      Transmit frame corruption due to AHB error */
        uint32_t: 1;          /*!< bit   28      Reserved */
uint32_t RLERR :
        1;   /*!< bit   29      Retry limit exceeded, transmit error detected */
uint32_t WRAP :
        1;    /*!< bit   30      Marks last descriptor in transmit buffer descriptor list */
        uint32_t OWN : 1;     /*!< bit   31      Ownership of this buffer */
    } bit;
    uint32_t reg[2];
} GMAC_TX_DESC;

/**
    \brief The internal Ethernet interface.

    The ClearCore's internal interface structure, passed around as 'state'
    by the network interface for LwIP.
**/
typedef struct {
    GMAC_RX_DESC *rxDesc;       /* Receive buffer descriptor address */
    GMAC_TX_DESC *txDesc;       /* Transmit buffer descriptor address */
    uint8_t *rxBuffIndex;       /* Receive buffer index */
    uint16_t *txBuffIndex;      /* Transmit buffer index */
    uint8_t mac[6];             /* MAC address */
} ethInt;

typedef struct netif netInt;
typedef struct pbuf packetBuf;

#endif // !__ETHERNETAPI_H__
#endif // !HIDE_FROM_DOXYGEN