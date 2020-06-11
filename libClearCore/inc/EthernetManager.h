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
    \file EthernetManager.h
    This class controls access to the Ethernet Port device.

    It will allow you to set up:
        - Ethernet connections
**/

#ifndef __ETHERNETMANAGER_H__
#define __ETHERNETMANAGER_H__

#include <cstring>
#include <sam.h>
#include "EthernetApi.h"
#include "HardwareMapping.h"
#include "IpAddress.h"
#include "Phy.h"
#include "SysUtils.h"
#include "lwip/dhcp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

namespace ClearCore {

/**
    \brief ClearCore Ethernet configuration manager

    This class manages setup and access to the Ethernet PHY chip and Ethernet
    Media Access Controller (GMAC) peripheral.

    For more detailed information on the ClearCore Ethernet system, check out
    the \ref EthernetMain informational page.
**/
class EthernetManager {
    friend class SysManager;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for singleton instance
    **/
    static EthernetManager &Instance();

    /**
        \brief Initialize the EthernetManager.

        Configures the GMAC pins and settings. Including addressing GMAC
        registers to point to descriptor buffers and selecting interrupts
        to activate.

        \note Disables transmit and receive.
    **/
    void Initialize();

    /**
        \brief Initialize the PHY.

        Performs a software reset of the PHY and configures the default
        settings. Can be used to re-initialize when PHY initialization fails.

        \code{.cpp}
        if (EthernetMgr.PhyInitFailed()) {
            // PHY initialization failed, re-initialize.
            EthernetMgr.PhyInitialize();
        }
        \endcode
    **/
    void PhyInitialize();

    /**
        \brief Read or write to a PHY register.

        Initiates and completes a shift operation to the PHY via the GMAC's
        management port.

        \param[in] phyOp    The PHY operation (read or write).
        \param[in] phyReg   The PHY register to read the contents of.
        \param[in] contents The desired contents for a write operation,
                            ignored for read.

        \return Returns the contents of the PHY register after the read or write
    **/
    uint32_t PhyShift(uint32_t phyOp, uint32_t phyReg, uint32_t contents);

    /**
        \brief Read and return the contents of a PHY register.

        \param[in] phyReg   The PHY register to read the contents of.

        \return Returns the contents of the specified PHY register.
    **/
    uint32_t PhyRead(uint32_t phyReg);

    /**
        \brief Write to a PHY register.

        \param[in] phyReg   The PHY register that will be written.
        \param[in] contents The value to write into the PHY register.

        \return Returns the contents of the PHY register after the write.
    **/
    uint32_t PhyWrite(uint32_t phyReg, uint32_t contents);
#endif

    /**
        \brief Check the link status from the PHY.

        \return Returns true if the PHY has indicated link up.
        Returns false if the PHY has indicated link down.

        \note The PHY link should be confirmed before activating transmit and
        receive of frames through the GMAC.
    **/
    volatile const bool &PhyLinkActive() {
        return m_phyLinkUp;
    }

    /**
        \brief Check the remote fault status from the PHY.

        \return Returns true if the PHY indicates a remote fault.
    **/
    volatile const bool &PhyRemoteFault() {
        return m_phyRemoteFault;
    }

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Check the initialization failure status of the PHY.

        \return Returns true if the PHY failed to initialize.
    **/
    volatile const bool &PhyInitFailed() {
        return m_phyInitFailed;
    }

    /**
        \brief Enable transmit and receive of frames.

        \param[in] enable True enables transmit and receive of frames.
    **/
    void Enable(bool enable);

    /**
        \brief Clear on read check if a frame was received.

        \return Returns true if a frame was received.
    **/
    bool ReceivedFrameFlag();

    /**
        Interrupt handler for Ethernet PHY.
    **/
    void IrqHandlerPhy();

    /**
        \brief Interrupt handler for Ethernet GMAC.

        \note The interrupt register and bits are cleared on read.
    **/
    void IrqHandlerGmac();
#endif

    /**
        \brief Get the MAC address.

        \return Returns the MAC address as uint8_t pointer type.
    **/
    uint8_t *MacAddress();

    /**
        \brief Get the local IP address.

        \return Returns the local IP address.
    **/
    IpAddress LocalIp();

    /**
        \brief Set the local IP address.

        \note The local IP must be provided when using a static local IP. In
        this case, be sure to specify a valid IP address on the same network
        as your router/switch. This is probably an address of the form
        "192.168.x.y". For example, if your router's address is 192.168.0.1,
        you should use an address of the form 192.168.0.y as long as there
        isn't a device on the network already that was assigned the same IP
        address.
        \note Doesn't have any effect when using DHCP.
    **/
    void LocalIp(IpAddress ipaddr);

    /**
        \brief Get the netmask IP.

        \return Returns the netmask IP as uint32_t type.
    **/
    IpAddress NetmaskIp();

    /**
        \brief Set the netmask IP.
        \note Doesn't have any effect when using DHCP.

        \note The netmask IP must be provided when using a static local IP.
    **/
    void NetmaskIp(IpAddress address);

    /**
        \brief Get the gateway IP address.

        \return Returns the gateway IP address.
    **/
    IpAddress GatewayIp();

    /**
        \brief Set the gateway IP address.
        \note The gateway IP must be provided when using a static local IP.
        \note Doesn't have any effect when using DHCP.
    **/
    void GatewayIp(IpAddress address);

    /**
        \brief Get the DNS IP address used for address resolution.

        \return Returns the current default DNS IP address.
    **/
    IpAddress DnsIp();

    /**
        \brief Set the DNS IP address used for address resolution.

        \param[in] dns The DNS IP address.

        \note Doesn't have any effect when called prior to calling Setup().
    **/
    void DnsIp(IpAddress dns);

#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Get a pointer to the network interface struct.

        \return Returns a pointer to the network interface struct used by LwIP.
    **/
    netif *MacInterface() {
        return &m_macInterface;
    }
#endif

    /**
        \brief Get the retransmission timeout.

        \return Returns the retransmission timeout in milliseconds.
    **/
    volatile const uint16_t &RetransmissionTimeout() {
        return m_retransmissionTimeout;
    }

    /**
        \brief Set the retransmission timeout.

        \param[in] timeout The retransmission timeout interval to be set,
        in milliseconds.
    **/
    void RetransmissionTimeout(uint8_t timeout) {
        m_retransmissionTimeout = timeout;
    }

    /**
        \brief Get the retransmission count.

        \return The number of times to attempt transmitting before giving up.
    **/
    volatile const uint8_t &RetransmissionCount() {
        return m_retransmissionCount;
    }

    /**
        \brief Set the retransmission count.

        \param[in] count The retransmission count to be set.

        \note This is really the total transmission count, so you should never
              set this to a value less than 1.
    **/
    void RetransmissionCount(uint8_t count) {
        m_retransmissionCount = count;
    }

    /**
        \brief Set up DHCP connection to retrieve local IP.

        Attempts to perform DHCP negotiation to be supplied an IP address.

        \return Returns true if DHCP supplied an IP address.
    **/
    bool DhcpBegin();

    /**
        \brief Setup LwIP with the local network interface.
        \note Should only be called once.
    **/
    void Setup();

    /**
        \brief Perform any necessary periodic Ethernet and LwIP updates.

        Sends all incoming, buffered packets to the LwIP interface. Calls
        sys_check_timeouts() to perform any necessary LwIP related tasks.

        \note Must be called regularly when actively using Ethernet.
        \note Must NOT be called from an interrupt context.
    **/
    void Refresh();

    /**
        \brief A flag to indicate whether Ethernet setup has been invoked.

        \return true if Ethernet has been set up, false otherwise.
    **/
    volatile const bool &EthernetActive() {
        return m_ethernetActive;
    }

private:
    // GMAC port/pin combinations
    uint32_t m_portPhyTxen;
    uint32_t m_pinPhyTxen;
    uint32_t m_portPhyTxd0;
    uint32_t m_pinPhyTxd0;
    uint32_t m_portPhyTxd1;
    uint32_t m_pinPhyTxd1;
    uint32_t m_portPhyRxd0;
    uint32_t m_pinPhyRxd0;
    uint32_t m_portPhyRxd1;
    uint32_t m_pinPhyRxd1;
    uint32_t m_portPhyRxer;
    uint32_t m_pinPhyRxer;
    uint32_t m_portPhyRxdv;
    uint32_t m_pinPhyRxdv;
    uint32_t m_portPhyMdio;
    uint32_t m_pinPhyMdio;
    uint32_t m_portPhyMdc;
    uint32_t m_pinPhyMdc;
    uint32_t m_portPhyTxclk;
    uint32_t m_pinPhyTxclk;
    uint32_t m_portPhyInt;
    uint32_t m_pinPhyInt;

    uint32_t m_phyExtInt;
    // PHY link up bit - updated via PHY interrupt
    bool m_phyLinkUp;
    // PHY remote fault bit -- updated via PHY interrupt
    bool m_phyRemoteFault;
    // PHY initialization failed status
    bool m_phyInitFailed;

    // received a frame flag
    bool m_recv;
    // DHCP flag
    bool m_dhcp;
    // Ethernet setup complete flag
    bool m_ethernetActive;

    // Receive Buffer Current Index
    uint8_t m_rxBuffIndex;
    // Transmit Buffer Current Index
    uint16_t m_txBuffIndex;
    // Receive Buffer Descriptor List
    GMAC_RX_DESC m_rxDesc[RX_BUFF_CNT];
    // Transmit Buffer Descriptor List
    GMAC_TX_DESC m_txDesc[TX_BUFF_CNT];
    // Receive Buffers
    uint8_t m_rxBuffer[RX_BUFF_CNT][RX_BUFFER_SIZE];
    // Transmit Buffers
    uint8_t m_txBuffer[TX_BUFF_CNT][TX_BUFFER_SIZE];

    // Blocking retransmission timeout in milliseconds
    uint16_t m_retransmissionTimeout;
    // Number of transmission attempts before giving up
    uint8_t m_retransmissionCount;

    // internal network interface
    ethInt m_ethernetInterface;

    /* Built-in lwIP types.
     */
    // lwIP's network interface
    struct netif m_macInterface;
    // DHCP configuration
    struct dhcp *m_dhcpData;

    /**
        \brief Network interface initialization for LwIP.
        \note Should only be called once.
    **/
    void NetifInit();

    /**
        \brief Setup a single GMAC GPIO.

        Sets a GPIO pin to enabled and assigns the correct PMUX
        (odd or even) to PER_GMAC to be controlled by the GMAC.

        \param[in] port The port.
        \param[in] pin  The pin.
    **/
    void ConfigureGpioPerGmac(uint32_t port, uint32_t pin);

    /**
        Construct
    **/
    EthernetManager();

}; // EthernetManager

} // ClearCore namespace

#endif // !__ETHERNETMANAGER_H__
