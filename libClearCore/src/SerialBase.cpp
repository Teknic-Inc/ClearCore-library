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
    Serial implementation

    ClearCore Serial Connector Base Class.
**/

#include "SerialBase.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sam.h>
#include "atomic_utils.h"
#include "InputManager.h"
#include "SysTiming.h"
#include "SysUtils.h"

namespace ClearCore {

#define __SERCOM_USART_CLOCK_INDEX 0
#define __SERCOM_SPI_CLOCK_INDEX 7
#define __SERCOM_USART_CLOCK (SystemCoreClock)
#define __SERCOM_SPI_CLOCK (10000000) // __CLEARCORE_GCLK7_HZ

// Dummy location to read/write SPI data that is unused
static uint32_t spiDummy;

extern volatile uint32_t tickCnt;
extern InputManager &InputMgr;

/**
    Construct this instance and remember all the pads and bit locations.
**/
SerialBase::SerialBase(const PeripheralRoute *ctsMisoInfo,
                       const PeripheralRoute *rtsSsInfo,
                       const PeripheralRoute *rxSckInfo,
                       const PeripheralRoute *txMosiInfo,
                       uint8_t peripheral)
    : m_parity(PARITY_N),
      m_stopBits(1),
      m_charSize(8),
      m_portMode(PortModes::UART),
      m_polarity(SCK_LOW),
      m_phase(LEAD_CHANGE),
      m_ssMode(LINE_OFF),
      m_rtsMode(LINE_HW),
      m_flowControl(false),
      m_ctsMisoInfo(ctsMisoInfo),
      m_rtsSsInfo(rtsSsInfo),
      m_rxSckInfo(rxSckInfo),
      m_txMosiInfo(txMosiInfo),
      m_baudRate(9600),
      m_peripheral(peripheral),
      m_portOpen(false),
      m_serialBreak(false),
      m_dreIrqN(static_cast<IRQn_Type>(INT32_MAX)),
      m_dmaRxChannel(DMA_INVALID_CHANNEL),
      m_dmaTxChannel(DMA_INVALID_CHANNEL),
      m_bufferIn{0}, m_bufferOut{0},
      m_inHead(0), m_inTail(0),
      m_outHead(0), m_outTail(0) {
    static Sercom *const sercom_instances[SERCOM_INST_NUM] = SERCOM_INSTS;
    m_serPort = sercom_instances[ctsMisoInfo->sercomNum];
}

/**
    Enable/Disable the port.

    ENABLE is the same for all modes of the port SPI/USART/I2C
**/
void SerialBase::PortEnable(bool initializing) {
    // Enable port and wait for the sync.
    m_serPort->USART.CTRLA.bit.ENABLE = 1;
    SercomUsart *usart = &m_serPort->USART;
    SYNCBUSY_WAIT(usart, SERCOM_USART_SYNCBUSY_ENABLE);

    if (!initializing) {
        PMUX_ENABLE(m_txMosiInfo->gpioPort, m_txMosiInfo->gpioPin);
    }
}

void SerialBase::PortDisable() {
    // Disable port and wait for the sync.
    PMUX_DISABLE(m_txMosiInfo->gpioPort, m_txMosiInfo->gpioPin);
    if (m_serPort->USART.CTRLA.bit.ENABLE) {
        m_serPort->USART.CTRLA.bit.ENABLE = 0;
    }

    SercomUsart *usart = &m_serPort->USART;
    SYNCBUSY_WAIT(usart, SERCOM_USART_SYNCBUSY_ENABLE);

    Flush();
    FlushInput();
}

/**
    Disable the port and close it.
**/
void SerialBase::PortClose() {
    if (m_portOpen) {
        // Flush the transmit buffer before closing
        WaitForTransmitIdle();

        DATA_DIRECTION_INPUT(m_rtsSsInfo->gpioPort, 1L << m_rtsSsInfo->gpioPin);
        PortDisable();
        m_portOpen = false;
    }
}

void SerialBase::WaitOneCharTime() {
    uint32_t tickVal = tickCnt;
    uint32_t ticksNeeded =
        max(2, (SampleRateHz * 10 + m_baudRate - 1) / m_baudRate);
    while (tickCnt - tickVal < ticksNeeded) {
        continue;
    }
}

void SerialBase::FlowControl(bool useFlowControl) {
    m_flowControl = useFlowControl;
    if (m_portMode == UART && m_portOpen) {
        // Defines the Transmit Data Pin Out
        // 0x0 for TxD pin PAD 0, RTS pin N/A, CTS pin N/A
        // 0x2 for TxD pin PAD 0, RTS pin PAD 2, CTS pin PAD 3
        bool sercomEnabled = m_serPort->USART.CTRLA.bit.ENABLE;
        PortDisable();
        m_serPort->USART.CTRLA.bit.TXPO = m_flowControl ? 2 : 0;
        if (sercomEnabled) {
            PortEnable();
        }
    }
}

bool SerialBase::RtsMode(CtrlLineModes mode) {
    m_rtsMode = mode;
    return RtsSsPinState(mode);
}

bool SerialBase::CtsState() {
    return !(*InputMgr.m_inputPtrs[m_ctsMisoInfo->gpioPort] &
             (1UL << m_ctsMisoInfo->gpioPin));
}

bool SerialBase::RtsSsPinState(CtrlLineModes mode) {
    bool success = true;

    switch (mode) {
        case LINE_HW:
            PMUX_ENABLE(m_rtsSsInfo->gpioPort, m_rtsSsInfo->gpioPin);
            break;
        case LINE_OFF:
        // Fall through
        case LINE_ON:
            DATA_OUTPUT_STATE(m_rtsSsInfo->gpioPort,
                              1L << m_rtsSsInfo->gpioPin,
                              mode == LINE_OFF); // ON is low voltage due to
            // inversions
            PMUX_DISABLE(m_rtsSsInfo->gpioPort, m_rtsSsInfo->gpioPin);
            break;
        default:
            // Should not happen
            success = false;
            break;
    }

    return success;
}

/**
    Initiate a serial break
**/
void SerialBase::SerialBreak(bool enable) {
    if (enable) {
        // Disable pmux on tx out
        PMUX_DISABLE(m_txMosiInfo->gpioPort, m_txMosiInfo->gpioPin);
        DATA_OUTPUT_STATE(m_txMosiInfo->gpioPort,
                          1L << m_txMosiInfo->gpioPin, false);   // break
        WaitOneCharTime();
    }
    else {
        // Enable pmux on tx out
        PMUX_ENABLE(m_txMosiInfo->gpioPort, m_txMosiInfo->gpioPin);
        DATA_OUTPUT_STATE(m_txMosiInfo->gpioPort,
                          1L << m_txMosiInfo->gpioPin, true);    // idle
    }
    m_serialBreak = enable;
    // Make sure the line state is held steady long enough so that
    // the enter/exit of break is not interpreted as a character
    WaitOneCharTime();
}

/**
    Open the port and set up the mode
**/
void SerialBase::PortOpen() {
    if (!m_portOpen) {
        m_portOpen = true;
        PortMode(m_portMode);
    }
}

/**
    Return if the port is open or not.
**/
bool SerialBase::PortIsOpen() {
    return m_portOpen;
}

/**
    Set the SERCOM up for the supported operational mode.

    This includes port setup, pad setup and clocks.
**/
bool SerialBase::PortMode(PortModes newMode) {
    if (newMode != SPI && newMode != UART) {
        return false;
    }
    m_portMode = newMode;
    // If port is not open just return
    if (!m_portOpen) {
        return true;
    }

    SercomUsart *usart = &m_serPort->USART;
    // Reset port to power-on state and force port to "disable"
    usart->CTRLA.bit.SWRST = 1;
    SYNCBUSY_WAIT(usart, SERCOM_USART_SYNCBUSY_SWRST);

    Flush();
    FlushInput();

    // Dummy init to intercept potential error later
    IRQn_Type IdNvic = PendSV_IRQn;
    uint8_t clockId;
    uint8_t dmaRxTrigger = DMAC_CHCTRLA_TRIGSRC_DISABLE_Val;
    uint8_t dmaTxTrigger = DMAC_CHCTRLA_TRIGSRC_DISABLE_Val;

    // Map clock and interrupt
    if (m_serPort == SERCOM0) {
        m_dmaRxChannel = DMA_SERCOM0_SPI_RX;
        m_dmaTxChannel = DMA_SERCOM0_SPI_TX;
        dmaRxTrigger = SERCOM0_DMAC_ID_RX;
        dmaTxTrigger = SERCOM0_DMAC_ID_TX;
        clockId = SERCOM0_GCLK_ID_CORE;
        IdNvic = SERCOM0_0_IRQn;
    }
    else if (m_serPort == SERCOM2) {
        m_dmaRxChannel = DMA_INVALID_CHANNEL;
        m_dmaTxChannel = DMA_INVALID_CHANNEL;
        clockId = SERCOM2_GCLK_ID_CORE;
        IdNvic = SERCOM2_0_IRQn;
    }
    else if (m_serPort == SERCOM3) {
        m_dmaRxChannel = DMA_INVALID_CHANNEL;
        m_dmaTxChannel = DMA_INVALID_CHANNEL;
        clockId = SERCOM3_GCLK_ID_CORE;
        IdNvic = SERCOM3_0_IRQn;
    }
    else if (m_serPort == SERCOM4) {
        m_dmaRxChannel = DMA_INVALID_CHANNEL;
        m_dmaTxChannel = DMA_INVALID_CHANNEL;
        clockId = SERCOM4_GCLK_ID_CORE;
        IdNvic = SERCOM4_0_IRQn;
    }
    else if (m_serPort == SERCOM5) {
        m_dmaRxChannel = DMA_INVALID_CHANNEL;
        m_dmaTxChannel = DMA_INVALID_CHANNEL;
        clockId = SERCOM5_GCLK_ID_CORE;
        IdNvic = SERCOM5_0_IRQn;
    }
    else if (m_serPort == SERCOM7) {
        m_dmaRxChannel = DMA_SERCOM7_SPI_RX;
        m_dmaTxChannel = DMA_SERCOM7_SPI_TX;
        dmaRxTrigger = SERCOM7_DMAC_ID_RX;
        dmaTxTrigger = SERCOM7_DMAC_ID_TX;
        clockId = SERCOM7_GCLK_ID_CORE;
        IdNvic = SERCOM7_0_IRQn;
    }
    else {
        return false;
    }

    if (IdNvic == PendSV_IRQn) {
        // If we've reached this point there's a problem.
        return false;
    }

    // Setup TX Out to high to hold the idle state or low for SerialBreak.
    DATA_OUTPUT_STATE(m_txMosiInfo->gpioPort,
                      1L << m_txMosiInfo->gpioPin, true); // idle
    DATA_DIRECTION_OUTPUT(m_txMosiInfo->gpioPort, 1L << m_txMosiInfo->gpioPin);

    // Set up SERCOM registers based on the desired mode.
    switch (m_portMode) {
        case SPI:
            /* Data Register Empty Interrupt */
            NVIC_DisableIRQ((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_DRE_Pos));
            /* Receive Complete Interrupt */
            NVIC_DisableIRQ((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_RXC_Pos));
            /* Receive Start Interrupt and errors */
            NVIC_DisableIRQ((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_RXS_Pos));

            // Setting Generic Clock Generator 0 as the default source
            SET_CLOCK_SOURCE(clockId, __SERCOM_SPI_CLOCK_INDEX);
            // SPI master mode
            m_serPort->SPI.CTRLA.bit.MODE = 0x3;
            // Clock polarity, leading edge is rising edge
            m_serPort->SPI.CTRLA.bit.CPOL = m_polarity;
            // Clock phase, determines SPI transfer mode
            m_serPort->SPI.CTRLA.bit.CPHA = m_phase;
            // Frame format supported by SPI in slave mode
            m_serPort->SPI.CTRLA.bit.FORM = 0;
            // In master operation, DIPO is MISO
            m_serPort->SPI.CTRLA.bit.DIPO = m_ctsMisoInfo->sercomPadNum;
            // In master operation, DOPO is MOSI
            // Data out can be either Pad 0: DOPO = 0 or Pad 3: DOPO = 2
            m_serPort->SPI.CTRLA.bit.DOPO = m_txMosiInfo->sercomPadNum ? 2 : 0;
            // 8 bit char size
            m_serPort->SPI.CTRLB.bit.CHSIZE = m_charSize &
                                              SERCOM_SPI_CTRLB_CHSIZE_Msk;
            // MSB is transferred first
            m_serPort->SPI.CTRLA.bit.DORD = 0;
            // Set buffer overflow immediately
            m_serPort->SPI.CTRLA.bit.IBON = 1;
            // Hardware controlled SS if m_ssMode = LINE_HW
            m_serPort->SPI.CTRLB.bit.MSSEN = 1;
            // Receiver enable
            m_serPort->SPI.CTRLB.bit.RXEN = 1;

            // Configure the Slave Select pin
            PMUX_SELECTION(m_rtsSsInfo->gpioPort, m_rtsSsInfo->gpioPin,
                           m_peripheral);
            SpiSsMode(m_ssMode);

            // Setup the DMA descriptors to perform asynchronous SPI transfers
            if (m_dmaRxChannel != DMA_INVALID_CHANNEL &&
                    m_dmaTxChannel != DMA_INVALID_CHANNEL) {
                DmacChannel *channel;
                DmacDescriptor *baseDesc;
                // Rx channel setup
                channel = DmaManager::Channel(m_dmaRxChannel);
                // Disable and reset the channel so it is clean to setup
                channel->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
                channel->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
                // Wait for the reset to finish
                while (channel->CHCTRLA.reg == DMAC_CHCTRLA_SWRST) {
                    continue;
                }

                channel->CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(dmaRxTrigger) |
                                       DMAC_CHCTRLA_TRIGACT_BURST |
                                       DMAC_CHCTRLA_BURSTLEN_SINGLE;

                // Set up the Rx source descriptor since that will not change
                baseDesc = DmaManager::BaseDescriptor(m_dmaRxChannel);
                baseDesc->DESCADDR.reg = static_cast<uint32_t>(0);
                baseDesc->SRCADDR.reg = (uint32_t)&m_serPort->SPI.DATA.reg;

                // Tx channel setup
                channel = DmaManager::Channel(m_dmaTxChannel);
                // Disable and reset the channel so it is clean to setup
                channel->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
                channel->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
                // Wait for the reset to finish
                while (channel->CHCTRLA.reg == DMAC_CHCTRLA_SWRST) {
                    continue;
                }

                channel->CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(dmaTxTrigger) |
                                       DMAC_CHCTRLA_TRIGACT_BURST |
                                       DMAC_CHCTRLA_BURSTLEN_SINGLE;

                // Set up the Tx dest descriptor since that will not change
                baseDesc = DmaManager::BaseDescriptor(m_dmaTxChannel);
                baseDesc->DESCADDR.reg = static_cast<uint32_t>(0);
                baseDesc->DSTADDR.reg = (uint32_t)&m_serPort->SPI.DATA.reg;
            }
            break;

        case UART:
        default:
            // 0x1 selects USART with internal clock
            usart->CTRLA.bit.MODE = 1;
            // 0x0 for 16x oversampling using arithmetic baud rate generation
            usart->CTRLA.bit.SAMPR = 0;
            // 0x0 for asynchronous communication
            usart->CTRLA.bit.CMODE = 0;

            // Defines the Receive Data Pin Out
            // 0x1 for SERCOM PAD 1
            usart->CTRLA.bit.RXPO = 1;

            FlowControl(m_flowControl);

            // 0x0 for 8-bit character size
            usart->CTRLB.bit.CHSIZE =
                m_charSize & SERCOM_USART_CTRLB_CHSIZE_Msk;
            // 0x1 for specifying LSB first data order
            usart->CTRLA.bit.DORD = 1;

            // Check what parity should be used (if any)
            if (m_parity != PARITY_N) {
                // 0x1 for USART frame format with parity
                usart->CTRLA.bit.FORM = 1;
                // Set the parity mode
                usart->CTRLB.bit.PMODE = m_parity;
            }
            else {
                // 0x0 for USART frame without parity
                usart->CTRLA.bit.FORM = 0;
            }

            // 0x0 sets one stop bit
            usart->CTRLB.bit.SBMODE = m_stopBits - 1;
            // 0x1 enables the USART receiver
            usart->CTRLB.bit.RXEN = 1;
            // 0x1 enables the USART transmitter
            usart->CTRLB.bit.TXEN = 1;
            // 0x0 Disables start of frame detection
            usart->CTRLB.bit.SFDE = 0;

            // Enable Error (ERROR) and Receive complete (RXC) interrupts
            usart->INTENSET.reg =
                SERCOM_USART_INTENSET_RXC |  SERCOM_USART_INTENSET_ERROR;

            // Sync CTRLB
            SYNCBUSY_WAIT(usart, SERCOM_USART_SYNCBUSY_CTRLB);

            // Setting NVIC

            /* Data Register Empty Interrupt */
            m_dreIrqN = (IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_DRE_Pos);
            NVIC_EnableIRQ((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_DRE_Pos));
            NVIC_SetPriority((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_DRE_Pos),
                             SERCOM_NVIC_TX_PRIORITY);

            /* Receive Complete Interrupt */
            NVIC_EnableIRQ((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_RXC_Pos));
            NVIC_SetPriority((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_RXC_Pos),
                             SERCOM_NVIC_RX_PRIORITY);

            /* Receive Start Interrupt, CTS, and errors */
            NVIC_EnableIRQ((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_RXS_Pos));
            NVIC_SetPriority((IRQn_Type)(IdNvic + SERCOM_USART_INTFLAG_RXS_Pos),
                             SERCOM_NVIC_ERR_PRIORITY);

            PMUX_SELECTION(m_rtsSsInfo->gpioPort, m_rtsSsInfo->gpioPin,
                           m_peripheral);
            RtsMode(m_rtsMode);

            // Setting Generic Clock Generator 0 as the default source
            SET_CLOCK_SOURCE(clockId, __SERCOM_USART_CLOCK_INDEX);
            break;
    }

    // Calculating BAUD according to figure 33-2 in datasheet
    Speed(m_baudRate);

    // Enable the MUX on each of the pins
    DATA_DIRECTION_OUTPUT(m_rtsSsInfo->gpioPort, 1L << m_rtsSsInfo->gpioPin);

    PMUX_SELECTION(m_ctsMisoInfo->gpioPort, m_ctsMisoInfo->gpioPin,
                   m_peripheral);
    PIN_CONFIGURATION(m_ctsMisoInfo->gpioPort, m_ctsMisoInfo->gpioPin,
                      PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN);
    DATA_DIRECTION_INPUT(m_ctsMisoInfo->gpioPort, 1L << m_ctsMisoInfo->gpioPin);

    PMUX_SELECTION(m_rxSckInfo->gpioPort, m_rxSckInfo->gpioPin,
                   m_peripheral);
    PIN_CONFIGURATION(m_rxSckInfo->gpioPort, m_rxSckInfo->gpioPin,
                      PORT_PINCFG_PMUXEN);

    PMUX_SELECTION(m_txMosiInfo->gpioPort, m_txMosiInfo->gpioPin,
                   m_peripheral);
    PIN_CONFIGURATION(m_txMosiInfo->gpioPort, m_txMosiInfo->gpioPin,
                      PORT_PINCFG_PMUXEN);

    // Enable SERCOM and wait for the sync
    PortEnable(true);

    return true;
}

/**
    Change the baud rate for the port.
**/
bool SerialBase::Speed(uint32_t bitsPerSecond) {
    bool success = true;
    m_baudRate = bitsPerSecond;
    bool sercomEnabled = m_serPort->USART.CTRLA.bit.ENABLE;
    PortDisable();

    uint32_t baudVal;

    switch (m_portMode) {
        case SPI:
            // Calculate the maximum baud rate that is <= the target rate
            baudVal = ((__SERCOM_SPI_CLOCK / 2 + (bitsPerSecond - 1)) /
                       bitsPerSecond) - 1;
            // Limit the value to UINT8_MAX, even if it gives a rate faster
            // than our target
            if (baudVal > UINT8_MAX) {
                baudVal = UINT8_MAX;
                // If baudVal is greater than 255, a lower input clock rate is
                // needed to achieve the requested baud rate
                // We'll clip to the highest rate possible and let the user
                // know that their setting didn't take
                success = false;
            }
            m_serPort->SPI.BAUD.bit.BAUD = static_cast<uint8_t>(baudVal);
            break;

        case UART:
        default:
            float baudFloat =
                65536 * (1 - (16.0 * bitsPerSecond / __SERCOM_USART_CLOCK));
            m_serPort->USART.BAUD.bit.BAUD = (uint16_t)(baudFloat + 0.5);
            break;
    }

    if (sercomEnabled) {
        PortEnable();
    }
    return success;
}

/**
    Set UART transmission parity format.
**/
bool SerialBase::Parity(Parities newParity) {
    m_parity = newParity;

    switch (m_portMode) {
        case SPI:
            return false;
        case UART:
        default: {
            bool sercomEnabled = m_serPort->USART.CTRLA.bit.ENABLE;
            PortDisable();
            if (m_parity != PARITY_N) {
                // 0x1 to set a USART frame with parity
                m_serPort->USART.CTRLA.bit.FORM = 1;
                // Set the parity
                m_serPort->USART.CTRLB.bit.PMODE = m_parity;
            }
            else {
                // 0x0 to set a USART frame without parity
                m_serPort->USART.CTRLA.bit.FORM = 0;
            }
            if (sercomEnabled) {
                PortEnable();
            }
            return true;
        }
    }
}

/**
    Change the number of bits in a character.
**/
bool SerialBase::CharSize(uint8_t size) {
    // Note: Supports 5,6,7,8,9 bits.
    if (size < 5 || size > 9) {
        return false;
    }
    else if (m_portMode == SPI && (size < 8 || size > 9)) {
        // Note: SPI only supports 8 or 9 bit characters
        return false;
    }
    m_charSize = size;
    bool sercomEnabled = m_serPort->USART.CTRLA.bit.ENABLE;
    PortDisable();


    switch (m_portMode) {
        case SPI:
            // Register size clips 8 and 9 to 0 and 1 to match register
            // definition.
            m_serPort->SPI.CTRLB.bit.CHSIZE = size &
                                              SERCOM_SPI_CTRLB_CHSIZE_Msk;
            break;
        case UART:
        default:
            // Register size clips 8 and 9 to 0 and 1 to match register
            // definition.
            m_serPort->USART.CTRLB.bit.CHSIZE = size &
                                                SERCOM_USART_CTRLB_CHSIZE_Msk;
            break;
    }
    if (sercomEnabled) {
        PortEnable();
    }
    return true;
}

/**
    Change the number of stop bits used in UART communication.
**/
bool SerialBase::StopBits(uint8_t bits) {
    if (bits < 1 || bits > 2) {
        return false;
    }
    m_stopBits = bits;
    // Don't apply the change yet if we are not in UART mode
    if (m_portMode != PortModes::UART) {
        return true;
    }
    bool sercomEnabled = m_serPort->USART.CTRLA.bit.ENABLE;
    PortDisable();
    m_serPort->USART.CTRLB.bit.SBMODE = bits - 1;
    if (sercomEnabled) {
        PortEnable();
    }
    return true;
}

/**
    Change the data order for the port.
**/
void SerialBase::DataOrder(DataOrders newOrder) {
    bool sercomEnabled = m_serPort->USART.CTRLA.bit.ENABLE;
    PortDisable();
    // SPI/USART use the same bit.
    m_serPort->USART.CTRLA.bit.DORD = newOrder;
    if (sercomEnabled) {
        PortEnable();
    }
}

/**
    Flush transmit buffers.
**/
void SerialBase::Flush() {
    // Flush buffers
    m_bufferOut[0] = 0;
    m_outTail = 0;
    m_outHead = 0;
}

/**
    Flush receive buffers.
**/
void SerialBase::FlushInput() {
    // Flush buffers
    m_bufferIn[0] = 0;
    m_inTail = 0;
    m_inHead = 0;
    EnableRxcInterruptUart();
}

/**
    Enable the Data Register Empty USART interrupt.
**/
void SerialBase::EnableDreInterruptUart() {
    m_serPort->USART.INTENSET.reg = SERCOM_USART_INTENSET_DRE;
}

/**
    Disable the Data Register Empty USART interrupt.
**/
void SerialBase::DisableDreInterruptUart() {
    m_serPort->USART.INTENCLR.reg = SERCOM_USART_INTENCLR_DRE;
}

/**
    Enable the Receive Complete USART interrupt.
**/
void SerialBase::EnableRxcInterruptUart() {
    m_serPort->USART.INTENSET.reg = SERCOM_USART_INTENSET_RXC;
}

/**
    Disable the Receive Complete USART interrupt.
**/
void SerialBase::DisableRxcInterruptUart() {
    m_serPort->USART.INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;
}

/**
    Set/Clear the slave select mode.
**/
bool SerialBase::SpiSsMode(CtrlLineModes mode) {
    m_ssMode = mode;
    return RtsSsPinState(mode);
}

/**
    Change the polarity and phase for the SPI clock.
**/
void SerialBase::SpiClock(
    SpiClockPolarities polarity, SpiClockPhases phase) {
    m_polarity = polarity;
    m_phase = phase;

    // If we are not currently in SPI mode, don't apply the change yet.
    if (m_portMode != PortModes::SPI) {
        return;
    }
    bool sercomEnabled = m_serPort->USART.CTRLA.bit.ENABLE;
    PortDisable();
    m_serPort->SPI.CTRLA.bit.CPOL = m_polarity;
    m_serPort->SPI.CTRLA.bit.CPHA = m_phase;
    if (sercomEnabled) {
        PortEnable();
    }
}

/**
    Attempt to get next character from serial channel.
**/
int16_t SerialBase::CharGet() {
    // Return if nothing is waiting.
    if (m_inTail == m_inHead) {
        return SerialBase::EOB;
    }

    // Get head of buffer, wrapped.
    int32_t nextIndex = NextIndex(m_inHead);
    // Get head character.
    int16_t returnChar = m_bufferIn[m_inHead];
    // Save new head ptr.
    m_inHead = nextIndex;
    EnableRxcInterruptUart();

    return returnChar;
}

/**
    Attempt to get next character from serial channel without pulling it
    out of the buffer.
**/
int16_t SerialBase::CharPeek() {
    // Return if nothing is waiting
    if (m_inTail == m_inHead) {
        return SerialBase::EOB;
    }

    // Get head character
    int16_t peekChar = m_bufferIn[m_inHead];
    return (peekChar);
}

/**
    Attempt to send a character on serial channel
**/
bool SerialBase::SendChar(uint8_t charToSend) {
    // Guard against sending to a closed port or an incorrect mode.
    if (!m_portOpen || m_portMode == PortModes::SPI) {
        return false;
    }
    // Calculate next location with wrap
    uint32_t nextIndex = NextIndex(m_outTail);

    // If the buffer is full, elevate the priority of the interrupt to drain
    // the buffer and wait for some space to open up
    while (nextIndex == m_outHead) {
        if (!m_portOpen) {
            return false;
        }
    }

    // Queue this character on tail
    m_bufferOut[m_outTail] = charToSend;
    m_outTail = nextIndex;

    EnableDreInterruptUart();
    return true;
}

/**
    SPI's TX and RX function
**/
uint8_t SerialBase::SpiTransferData(uint8_t data) {
    if (!m_portOpen || m_portMode != PortModes::SPI) {
        return 0;
    }
    // Write data into Data register
    m_serPort->SPI.DATA.bit.DATA = data;

    while (!m_serPort->SPI.INTFLAG.bit.RXC ||
            !m_serPort->SPI.INTFLAG.bit.TXC) {
        // If the port is not open, bail out
        if (!m_portOpen) {
            return 0;
        }
        // Wait for it to complete
        continue;
    }
    return m_serPort->SPI.DATA.bit.DATA;
}

/**
    SPI's multi-byte TX and RX function
**/
int32_t SerialBase::SpiTransferData(
    uint8_t const *writeBuf, uint8_t *readBuf, int32_t len) {
    if (!m_portOpen || m_portMode != SPI) {
        return 0;
    }

    int32_t iChar;
    for (iChar = 0; iChar < len; iChar++) {
        // Write data into Data register
        m_serPort->SPI.DATA.bit.DATA = writeBuf ? *writeBuf++ : 0;

        while (!m_serPort->SPI.INTFLAG.bit.RXC ||
                !m_serPort->SPI.INTFLAG.bit.TXC) {
            // If the port is not open, bail out
            if (!m_portOpen) {
                return iChar;
            }
            // Wait for it to complete
            continue;
        }

        if (readBuf) {
            *readBuf++ = m_serPort->SPI.DATA.bit.DATA;
        }
        else {
            // Read from the register (value unused).
            (void)m_serPort->SPI.DATA.bit.DATA;
        }
    }

    return iChar;
}

/**
    SPI's asynchronous multi-byte TX and RX function
**/
bool SerialBase::SpiTransferDataAsync(
    uint8_t const *writeBuf, uint8_t *readBuf, int32_t len) {
    if (!m_portOpen || m_portMode != SPI) {
        return false;
    }
    // Setup the DMA descriptors to perform asynchronous transfers
    if (m_dmaRxChannel == DMA_INVALID_CHANNEL ||
            m_dmaTxChannel == DMA_INVALID_CHANNEL) {
        return false;
    }
    DmacDescriptor *baseDesc;

    // Set up the Rx dest descriptor
    baseDesc = DmaManager::BaseDescriptor(m_dmaRxChannel);
    if (readBuf) {
        baseDesc->DSTADDR.reg = (uint32_t)(readBuf + len);
        baseDesc->BTCTRL.reg =
            DMAC_BTCTRL_BEATSIZE_BYTE | DMAC_BTCTRL_DSTINC | DMAC_BTCTRL_VALID;
    }
    else {
        baseDesc->DSTADDR.reg = (uint32_t)&spiDummy;
        baseDesc->BTCTRL.reg = DMAC_BTCTRL_BEATSIZE_BYTE | DMAC_BTCTRL_VALID;
    }
    baseDesc->BTCNT.reg = len;
    DmaManager::Channel(m_dmaRxChannel)->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

    // Set up the Tx source descriptor
    baseDesc = DmaManager::BaseDescriptor(m_dmaTxChannel);
    if (writeBuf) {
        baseDesc->SRCADDR.reg = (uint32_t)(writeBuf + len);
        baseDesc->BTCTRL.reg =
            DMAC_BTCTRL_BEATSIZE_BYTE | DMAC_BTCTRL_SRCINC | DMAC_BTCTRL_VALID;
    }
    else {
        baseDesc->SRCADDR.reg = (uint32_t)&spiDummy;
        baseDesc->BTCTRL.reg = DMAC_BTCTRL_BEATSIZE_BYTE | DMAC_BTCTRL_VALID;
    }
    baseDesc->BTCNT.reg = len;
    DmaManager::Channel(m_dmaTxChannel)->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

    return true;
}

bool SerialBase::SpiAsyncWaitComplete() {
    // If this channel is not set up to do DMA transfers, it is already done
    if (m_dmaRxChannel == DMA_INVALID_CHANNEL ||
            m_dmaTxChannel == DMA_INVALID_CHANNEL) {
        return true;
    }
    // The transfer is done when all of the Rx data has been read and the
    // channel disables
    while (m_portOpen && m_portMode == SPI &&
            DmaManager::Channel(m_dmaRxChannel)->CHCTRLA.bit.ENABLE) {
        continue;
    }
    return true;
}

void SerialBase::HandleFrameError() {
    // If there's a framing error raise a warning and clear the flag
    if (m_serPort->USART.STATUS.bit.FERR) {
        // Clear the hardware error flag
        m_serPort->USART.STATUS.bit.FERR = 1;
        // Set the error status bit
        m_errorRegAccum.bit.SerialFrameError = 1;
    }
}

void SerialBase::HandleParityError() {
    // If there's a parity error raise a warning and clear the flag
    if (m_serPort->USART.STATUS.bit.PERR) {
        // Clear the hardware error flag
        m_serPort->USART.STATUS.bit.PERR = 1;
        // Set the error status bit
        m_errorRegAccum.bit.SerialParityError = 1;
    }
}

void SerialBase::HandleOverflow() {
    // If there's an overflow error raise a warning and clear the flag
    if (m_serPort->USART.STATUS.bit.BUFOVF) {
        // Clear the hardware error flag
        m_serPort->USART.STATUS.bit.BUFOVF = 1;
        // Set the error status bit
        m_errorRegAccum.bit.SerialOverflowError = 1;
    }
}

SerialBase::SerialErrorStatusRegister SerialBase::ErrorStatusAccum(
    SerialErrorStatusRegister mask) {
    SerialErrorStatusRegister statusReg;
    statusReg.reg = atomic_fetch_and(&m_errorRegAccum.reg, ~mask.reg) & mask.reg;
    return statusReg;
}

/**
    Wait for the output to be idle
**/
void SerialBase::WaitForTransmitIdle() {
    if (m_portMode == UART) {
        // Wait until the out buffer has emptied
        while (m_outHead != m_outTail) {
            continue;
        }

        if (m_serPort->USART.INTFLAG.bit.DRE) {
            // Data register is already empty, no need to wait anymore
            return;
        }

        // Wait for transmission to complete
        while (!m_serPort->USART.INTFLAG.bit.TXC) {
            continue;
        }
    }
    else if (m_portMode == SPI) {
        SpiAsyncWaitComplete();
    }
}

/**
    Return the number of free characters in the receive buffer
**/
int32_t SerialBase::AvailableForRead() {
    int32_t difference = m_inTail - m_inHead;

    if (difference < 0) {
        return SERIAL_BUFFER_SIZE + difference;
    }
    else {
        return difference;
    }
}

/**
    Returns the number of available characters in the transmit buffer
**/
int32_t SerialBase::AvailableForWrite() {
    int32_t difference = m_outHead - m_outTail - 1;

    if (difference < 0) {
        difference += SERIAL_BUFFER_SIZE;
    }

    return difference;
}

// =========================== INTERRUPT API ===============================

/**
    Pulls characters from the DATA register and puts them in the rx buffer
**/
void SerialBase::RxProc() {
    // Must reinitialize to clear ort problems
    if (m_serPort->USART.RXERRCNT.reg != 0) {
        // On break detected, flush inBuf and insert flag character
        m_inTail = 0;
        m_inHead = 0;
        m_bufferIn[m_inTail++] = SerialBase::BREAK_DETECTED;

        // Clear error to allow more interrupts
        m_serPort->USART.INTFLAG.bit.ERROR = 1;
    }

    // Generate wrapped next location
    uint32_t nextIndex = NextIndex(m_inTail);
    while (m_serPort->USART.INTFLAG.bit.RXC && nextIndex != m_inHead) {
        m_bufferIn[m_inTail] = m_serPort->USART.DATA.bit.DATA;
        m_inTail = nextIndex;
        nextIndex = NextIndex(m_inTail);
    }
    if (nextIndex == m_inHead) {
        DisableRxcInterruptUart();
    }
}

/**
    Transmit any data waiting in the tx buffer
**/
void SerialBase::TxPump() {
    while (m_outHead != m_outTail) {
        if (!m_serPort->USART.INTFLAG.bit.DRE) {
            // Data register is full; can't send anything more right now
            return;
        }
        int32_t nextIndex = NextIndex(m_outHead);
        m_serPort->USART.DATA.bit.DATA = m_bufferOut[m_outHead];
        m_outHead = nextIndex;
    }

    DisableDreInterruptUart();
}

/**
    The TX data service interrupt handler.

    This should be called by SERCOMx_0 Interrupt Vector.
**/
void SerialBase::IrqHandlerTx() {
    switch (m_portMode) {
        case SPI:
            break;
        case UART:
        default:
            TxPump();
            break;
    }
}

/**
    Should be called by SERCOMx_1 Interrupt Vector.
**/
void SerialBase::IrqHandler1() {
    // Does nothing
}

/**
    Interrupt handler for the RX data service.

    This should be called by SERCOMx_2 Interrupt Vector.
**/
void SerialBase::IrqHandlerRx() {
    switch (m_portMode) {
        case SPI:
            break;
        case UART:
        default:
            RxProc();
            break;
    }
}

/**
    Interrupt handler for any serial port exceptions.

    Should be called by SERCOMx_3 Interrupt Vector.
**/
void SerialBase::IrqHandlerException() {
    switch (m_portMode) {
        case SPI:
            // This should not occur, but clear the interrupt flags to be safe.
            m_serPort->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_ERROR;
            break;
        case UART:
        default:
            if (m_serPort->USART.INTFLAG.bit.ERROR) {
                // Clear the interrupt flag
                m_serPort->USART.INTFLAG.reg = SERCOM_USART_INTFLAG_ERROR;
                HandleFrameError();
                HandleParityError();
                HandleOverflow();
            }
            break;
    }
}

} // ClearCore namespace