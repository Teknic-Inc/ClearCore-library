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
    EthernetManager implementation

    Implements an Ethernet port.
**/

#include "EthernetManager.h"
#include "ethernetif.c"
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/timeouts.h"
#include "NvmManager.h"
#include "SysTiming.h"

namespace ClearCore {

extern NvmManager &NvmMgr;

EthernetManager &EthernetMgr = EthernetManager::Instance();

EthernetManager &EthernetManager::Instance() {
    static EthernetManager *instance = new EthernetManager();
    return *instance;
}

EthernetManager::EthernetManager()
    : m_portPhyTxen(PHY_TXEN.gpioPort), m_pinPhyTxen(PHY_TXEN.gpioPin),
      m_portPhyTxd0(PHY_TXD0.gpioPort), m_pinPhyTxd0(PHY_TXD0.gpioPin),
      m_portPhyTxd1(PHY_TXD1.gpioPort), m_pinPhyTxd1(PHY_TXD1.gpioPin),
      m_portPhyRxd0(PHY_RXD0.gpioPort), m_pinPhyRxd0(PHY_RXD0.gpioPin),
      m_portPhyRxd1(PHY_RXD1.gpioPort), m_pinPhyRxd1(PHY_RXD1.gpioPin),
      m_portPhyRxer(PHY_RXER.gpioPort), m_pinPhyRxer(PHY_RXER.gpioPin),
      m_portPhyRxdv(PHY_RXDV.gpioPort), m_pinPhyRxdv(PHY_RXDV.gpioPin),
      m_portPhyMdio(PHY_MDIO.gpioPort), m_pinPhyMdio(PHY_MDIO.gpioPin),
      m_portPhyMdc(PHY_MDC.gpioPort), m_pinPhyMdc(PHY_MDC.gpioPin),
      m_portPhyTxclk(PHY_TXCLK.gpioPort), m_pinPhyTxclk(PHY_TXCLK.gpioPin),
      m_portPhyInt(PHY_INT.gpioPort), m_pinPhyInt(PHY_INT.gpioPin),
      m_phyExtInt(PHY_INT.extInt), m_phyLinkUp(false), m_phyRemoteFault(false),
      m_phyInitFailed(false), m_recv(false), m_dhcp(false), m_ethernetActive(false),
      m_rxBuffIndex(0), m_txBuffIndex(0), m_rxBuffer{0}, m_txBuffer{0},
      m_retransmissionTimeout(200), m_retransmissionCount(8),
      m_ethernetInterface({}), m_macInterface({}), m_dhcpData(nullptr) { }

/**
    Initialize the EthernetManager.
**/
void EthernetManager::Initialize() {
    // Disable transmit and receive circuits
    // Must be done prior to configuring GMAC settings
    Enable(false);

    // Write GMAC settings
    GMAC->NCR.bit.MPE = 1;          // Management port enabled
    GMAC->NCFGR.bit.SPD = 1;        // 100 Mbps
    GMAC->NCFGR.bit.FD = 1;         // Full duplex mode
    GMAC->NCFGR.bit.MAXFS = 1;      // Increase max frame size
    GMAC->NCFGR.bit.CLK = 0x04;     // MCK divided by 64
    GMAC->UR.bit.MII = 0;           // RMII mode
    GMAC->DCFGR.bit.FBLDO = 0x04;   // Use INCR4 AHB bursts
    GMAC->DCFGR.bit.RXBMS = 0x03;   // 4 Kbytes receiver packet buffer mem size
    GMAC->DCFGR.bit.TXPBMS = 0x01;  // 4 Kb transmitter packet buffer mem size
    GMAC->DCFGR.bit.DRBS = 0x02;    // 128 bytes receiver buffer in AHB
    GMAC->WOL.reg = 0;
    GMAC->IPGS.reg = GMAC_IPGS_FL((0x1 << 8) | 0x1);

    // Initialize Receive Descriptor List
    for (uint8_t buff = 0; buff < RX_BUFF_CNT; buff++) {
        // Write an appropriate address
        m_rxDesc[buff].reg[0] = (uint32_t) &m_rxBuffer[buff][0];
        // Clear out status
        m_rxDesc[buff].reg[1] = 0;
    }
    // Mark the last descriptor in the queue to wrap
    m_rxDesc[RX_BUFF_CNT - 1].bit.WRAP = 1;
    m_rxBuffIndex = 0;

    // Initialize Transmit Descriptor List
    for (uint8_t buff = 0; buff < TX_BUFF_CNT; buff++) {
        // Write an appropriate address
        m_txDesc[buff].reg[0] = (uint32_t) &m_txBuffer[buff][0];
        // Clear out status
        m_txDesc[buff].reg[1] = 0;
        // Mark the entries as used
        m_txDesc[buff].bit.OWN = 1;
        // Mark the entries as individual.
        m_txDesc[buff].bit.LB = 1;
    }
    // Mark the last descriptor in the queue to wrap
    m_txDesc[TX_BUFF_CNT - 1].bit.WRAP = 1;
    m_txBuffIndex = 0;

    // Write address of transmit buffer descriptor list to register
    // transmit buffer queue pointer.
    // *Must be done while transmit is disabled.
    GMAC->TBQB.reg = (uint32_t) &m_txDesc;
    // Write address of receive buffer descriptor list to register receive
    // buffer queue pointer.
    // *Must be done while receive is disabled.
    GMAC->RBQB.reg = (uint32_t) &m_rxDesc;

    // Reset interrupts
    NVIC_DisableIRQ(GMAC_IRQn);
    NVIC_ClearPendingIRQ(GMAC_IRQn);
    NVIC_EnableIRQ(GMAC_IRQn);

    // Configure the GPIO pins
    ConfigureGpioPerGmac(m_portPhyTxen, m_pinPhyTxen);
    ConfigureGpioPerGmac(m_portPhyTxd0, m_pinPhyTxd0);
    ConfigureGpioPerGmac(m_portPhyTxd1, m_pinPhyTxd1);
    ConfigureGpioPerGmac(m_portPhyRxd0, m_pinPhyRxd0);
    ConfigureGpioPerGmac(m_portPhyRxd1, m_pinPhyRxd1);
    ConfigureGpioPerGmac(m_portPhyRxer, m_pinPhyRxer);
    ConfigureGpioPerGmac(m_portPhyRxdv, m_pinPhyRxdv);
    ConfigureGpioPerGmac(m_portPhyMdio, m_pinPhyMdio);
    ConfigureGpioPerGmac(m_portPhyMdc, m_pinPhyMdc);
    ConfigureGpioPerGmac(m_portPhyTxclk, m_pinPhyTxclk);

    // Configure PHY interrupt in.
    PIN_CONFIGURATION(m_portPhyInt, m_pinPhyInt, PORT_PINCFG_INEN);

    // Connect PAD to External Interrupt device
    PMUX_SELECTION(m_portPhyInt, m_pinPhyInt, PER_EXTINT);

    PMUX_ENABLE(m_portPhyInt, m_pinPhyInt);
    PORT->Group[m_portPhyInt].PINCFG[m_pinPhyInt].bit.INEN = 1;

    // Enable appropriate GMAC interrupts.
    GMAC->IER.bit.TCOMP = 1;    // Transmit complete
    GMAC->IER.bit.RCOMP = 1;    // Receive complete

    // Set up EIC for PHY interrupts
    EIC->CTRLA.bit.ENABLE = 0;
    SYNCBUSY_WAIT(EIC, EIC_SYNCBUSY_ENABLE);

    uint32_t shiftAmt = 4 * (m_phyExtInt % 8);
    // Interrupt 3 (from peripheral routing)
    EIC->INTENSET.reg = (1UL << m_phyExtInt);
    // Set interrupt mode (CONFIG reg)
    EIC->CONFIG[m_phyExtInt / 8].reg &= (uint32_t) ~(0xf << shiftAmt);
    EIC->CONFIG[m_phyExtInt / 8].reg |=
        (EIC_CONFIG_SENSE0_LOW_Val << shiftAmt);

    EIC->CTRLA.bit.ENABLE = 1;
    SYNCBUSY_WAIT(EIC, EIC_SYNCBUSY_ENABLE);

    // Initialize the PHY
    PhyInitialize();

    // Set up fields in our internal interface
    m_ethernetInterface.rxDesc = &m_rxDesc[0];
    m_ethernetInterface.txDesc = &m_txDesc[0];
    m_ethernetInterface.rxBuffIndex = &m_rxBuffIndex;
    m_ethernetInterface.txBuffIndex = &m_txBuffIndex;

    // Retrieve the MAC address from NVM and write it to the interface
    NvmMgr.MacAddress(m_ethernetInterface.mac);
}

void EthernetManager::PhyInitialize() {
    // Reset PHY status values.
    m_phyLinkUp = false;
    m_phyInitFailed = false;
    m_phyRemoteFault = false;
    // Verify that the PHY is online.
    if (PhyRead(PHY_B_CTRL) == 0xFFFF) {
        m_phyInitFailed = true;
        return;
    }
    // Software reset the PHY.
    PhyWrite(PHY_B_CTRL, PHY_B_CTRL_RES);
    if (PhyRead(PHY_ICS) != 0) {
        m_phyInitFailed = true;
        return;
    }
    // Enable PHY interrupts for Link-Down, Link-Up, and Remote Fault.
    uint32_t phyIntMask = (PHY_ICS_LDEN | PHY_ICS_LUEN | PHY_ICS_RFEN);
    uint32_t phyIntValue = PhyWrite(PHY_ICS, phyIntMask);
    // Verify the interrupts were set correctly. Ignore the 8 LSB.
    if ((phyIntMask >> 8) != (phyIntValue >> 8)) {
        m_phyInitFailed = true;
        return;
    }
    m_phyInitFailed = false;
}

uint32_t EthernetManager::PhyShift(uint32_t phyOp, uint32_t phyReg,
                                   uint32_t contents) {
    // Enable the GMAC management port and initiate a shift operation to the
    // PHY. The PHY's status register is returned into the GMAC's MAN register.
    GMAC->NCR.bit.MPE = 1;
    GMAC->MAN.reg = GMAC_MAN_CLTTO |   // Clause 22 or 45 operation (1 is 22)
                    GMAC_MAN_OP(phyOp) |        // Read or write operation
                    GMAC_MAN_PHYA(0) |          // PHY address (default is 0)
                    GMAC_MAN_REGA(phyReg) |     // Register in the PHY to access
                    GMAC_MAN_WTN(0x2) |         // Must be written to '1' '0'
                    GMAC_MAN_DATA(contents);

    while (!((GMAC->NSR.reg & GMAC_NSR_IDLE) >> GMAC_NSR_IDLE_Pos)) {
        // Wait for the PHY write to finish
        continue;
    }
    GMAC->NCR.bit.MPE = 0;

    return GMAC_MAN_DATA(GMAC->MAN.reg);
}

uint32_t EthernetManager::PhyRead(uint32_t phyReg) {
    return PhyShift(PHY_READ_OP, phyReg, 0);
}

uint32_t EthernetManager::PhyWrite(uint32_t phyReg, uint32_t contents) {
    PhyShift(PHY_WRITE_OP, phyReg, contents);
    return PhyShift(PHY_READ_OP, phyReg, 0);
}

/**
    Initialization for LwIP's network interface.
**/
void EthernetManager::NetifInit() {
    struct netif *netif = &m_macInterface;
    ip_addr_t dummyIp = IPADDR4_INIT(0);

    netif_add(netif, &dummyIp, &dummyIp, &dummyIp,
              &m_ethernetInterface, ethernetif_init, ethernet_input);

    netif_set_default(netif);

    netif_set_link_up(netif);

    netif_set_up(netif);

    Enable(true);
}

/**
    Enable transmit and receive of frames.
**/
void EthernetManager::Enable(bool enable) {
    bool enabled = (GMAC->NCR.bit.TXEN && GMAC->NCR.bit.RXEN);

    if (enable == enabled) {
        // Nothing to do.
        return;
    }

    GMAC->NCR.bit.TXEN = enable ? 1 : 0;
    GMAC->NCR.bit.RXEN = enable ? 1 : 0;

    if (!enable) {
        // Writing TXEN to 0 on the GMAC resets the GMAC's transmit queue
        // pointer to the address of m_txBuffer[0].
        // Reset the TX descriptors to init state
        for (uint8_t i = 0; i < TX_BUFF_CNT; i++) {
            m_txDesc[i].bit.OWN = 1;
            m_txDesc[i].bit.LB = 1;
        }
        m_txBuffIndex = 0;
    }
}

/**
    Clear on read check if a frame was received.
**/
bool EthernetManager::ReceivedFrameFlag() {
    bool recv = m_recv;
    m_recv = false;
    return recv;
}

/**
    The PHY interrupt handler.
**/
void EthernetManager::IrqHandlerPhy() {
    // Read the PHY interrupt status register to get the correct bits
    // (checking just LinkUp here?)
    EIC->INTFLAG.reg = 1UL << m_phyExtInt;
    uint32_t phyRegData = PhyRead(PHY_ICS);
    // PHY Link-Up
    if (phyRegData & PHY_ICS_LU) {
        m_phyLinkUp = true;
        // Disable must be done before writing GMAC settings.
        bool enabled = (GMAC->NCR.bit.TXEN == 1);
        Enable(false);

        // Set the GMAC settings to match the PHY's negotiated settings.
        uint16_t phyMode = PhyRead(PHY_CTRL_1) & PHY_CTRL_AN_MSK;

        GMAC->NCFGR.bit.SPD = (phyMode & PHY_CTRL_AN_SPD_MSK) ? 1 : 0;
        GMAC->NCFGR.bit.FD = (phyMode & PHY_CTRL_AN_FD_MSK) ? 1 : 0;

        Enable(enabled);
    }

    // PHY Link-Down
    if (phyRegData & PHY_ICS_LD) {
        m_phyLinkUp = false;
    }
    // PHY Remote Fault
    if (phyRegData & PHY_ICS_RF) {
        m_phyRemoteFault = true;
        Enable(false);  // disable
    }
}


/**
    The GMAC interrupt handler.

    This should be called by GMAC Interrupt Vector.
    *Note: the register & bits are cleared on read
**/
void EthernetManager::IrqHandlerGmac() {
    // Transmit status reg
    volatile uint32_t tsr;
    // Receive status reg
    volatile uint32_t rsr;

    tsr = GMAC->TSR.reg;    // Transmit status register
    rsr = GMAC->RSR.reg;    // Receive  status register
    // Need to clear the ISR (clear on read)
    GMAC->ISR.reg;

    // Frame transmitted
    if (tsr & GMAC_TSR_TXCOMP) {
        // Clear the TSR reg
        GMAC->TSR.reg = tsr;
    }

    // Frame received, add a packet to packet buffer.
    if (rsr & GMAC_RSR_REC) {
        m_recv = true;
    }
    // Clear the RSR reg
    GMAC->RSR.reg = rsr;
}

uint8_t *EthernetManager::MacAddress() {
    return m_ethernetInterface.mac;
}

IpAddress EthernetManager::LocalIp() {
    return IpAddress(m_macInterface.ip_addr.addr);
}

void EthernetManager::LocalIp(IpAddress ipaddr) {
    if (!m_dhcp) {
        m_macInterface.ip_addr.addr = uint32_t(ipaddr);
    }
}

IpAddress EthernetManager::NetmaskIp() {
    return IpAddress(m_macInterface.netmask.addr);
}

void EthernetManager::NetmaskIp(IpAddress address) {
    if (!m_dhcp) {
        m_macInterface.netmask.addr = uint32_t(address);
    }
}

IpAddress EthernetManager::GatewayIp() {
    return IpAddress(m_macInterface.gw.addr);
}

void EthernetManager::GatewayIp(IpAddress address) {
    if (!m_dhcp) {
        m_macInterface.gw.addr = uint32_t(address);
    }
}

IpAddress EthernetManager::DnsIp() {
    if (!m_ethernetActive) {
        return IpAddress();
    }
    return IpAddress(dns_getserver(0)->addr);
}

void EthernetManager::DnsIp(IpAddress dns) {
    if (m_ethernetActive) {
        ip_addr_t dnsIp = IPADDR4_INIT(uint32_t(dns));
        dns_setserver(0, &dnsIp);
    }
}

/**
    Setup a single GMAC GPIO.
**/
void EthernetManager::ConfigureGpioPerGmac(uint32_t port, uint32_t pin) {
    PMUX_ENABLE(port, pin);
    PMUX_SELECTION(port, pin, PER_GMAC);
}

bool EthernetManager::DhcpBegin() {
    struct netif *netif = &m_macInterface;
    uint32_t DHCP_TIMEOUT_MS = 1500;

    bool dhcpSuccess = false;
    for (uint8_t i = 0; i < 5 && !dhcpSuccess; i++) {
        // Try to get our config info from a DHCP server
        err_t err = dhcp_start(netif);

        if (err == ERR_OK) {
            uint32_t startMs = Milliseconds();

            while (dhcp_supplied_address(netif) == 0) {
                if (Milliseconds() - startMs > DHCP_TIMEOUT_MS) {
                    // Timed out, stop the dhcp process.
                    dhcp_release_and_stop(netif);
                    break;
                }
                Refresh();
            }
            dhcpSuccess = (dhcp_supplied_address(netif) != 0);
        }
    }
    if (dhcpSuccess) {
        // Set up info from DHCP configuration
        m_dhcpData = netif_dhcp_data(netif);
    }
    m_dhcp = dhcpSuccess;
    return dhcpSuccess;
}

void EthernetManager::Setup() {
    // Setup can only occur once.
    if (m_ethernetActive) {
        return;
    }
    lwip_init();
    dns_init();
    NetifInit();
    m_ethernetActive = true;
}

void EthernetManager::Refresh() {
    while (true) {
        // Check for an available packet.
        struct pbuf *packet = low_level_input(&m_macInterface);
        if (packet == NULL) {
            break;
        }
        // Send the packet as input to LwIP.
        ethernetif_input(&m_macInterface, packet);
    }
    sys_check_timeouts();
}

} // ClearCore namespace