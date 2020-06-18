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
    \file SerialBase.h
    This class controls access to the Serial Port device in the ARM processor.

    It will allow you to set up:
        - Asynchronous Serial transfers
        - SPI transfers
**/

#ifndef __SERIALBASE_H__
#define __SERIALBASE_H__

#include <stdint.h>
#include <sam.h>
#include "DmaManager.h"
#include "ISerial.h"
#include "PeripheralRoute.h"

namespace ClearCore {

/** Size of the serial send and receive buffers, in bytes (64). **/
#ifndef SERIAL_BUFFER_SIZE
#define SERIAL_BUFFER_SIZE 64
#endif

/** Serial receive interrupt priority level. **/
#ifndef SERCOM_NVIC_RX_PRIORITY
#define SERCOM_NVIC_RX_PRIORITY (static_cast<IRQn_Type>(1))
#endif
/** Serial transmit interrupt priority level. **/
#ifndef SERCOM_NVIC_TX_PRIORITY
#define SERCOM_NVIC_TX_PRIORITY (static_cast<IRQn_Type>(1))
#endif
/** Serial error interrupt priority level. **/
#ifndef SERCOM_NVIC_ERR_PRIORITY
#define SERCOM_NVIC_ERR_PRIORITY (static_cast<IRQn_Type>(7))
#endif

/**
    \brief ClearCore ARM Serial Port base class

    This class is used to create a buffered serial port.
**/
class SerialBase : public ISerial {
    friend class TestIO;

public:
    /**
        Break detected 'character' placed in the character stream when
        the break condition has been detected.
    **/
    static const int16_t BREAK_DETECTED = int16_t(0xBDBD);

    /**
        No character available indicator.
    **/
    static const int16_t EOB = -1;

    /**
        \union SerialErrorStatusRegister

        A register to report errors detected on the serial port.
    **/
    union SerialErrorStatusRegister {
        /**
            Broad access to the whole register
        **/
        uint32_t reg;

        /**
            Field access to the register
        **/
        struct {
            /**
                A frame error has been detected on the serial port.
            **/
            uint32_t SerialFrameError      : 1;
            /**
                A parity error has been detected on the serial port.
            **/
            uint32_t SerialParityError     : 1;
            /**
                An overflow error has been detected on the serial port.
            **/
            uint32_t SerialOverflowError   : 1;
        } bit;

        /**
            Serial error register default constructor
        **/
        SerialErrorStatusRegister() {
            reg = 0;
        }

        /**
            Serial error register constructor with initial value
        **/
        SerialErrorStatusRegister(uint32_t val) {
            reg = val;
        }

        /**
             Interpret the serial error register as a boolean by reporting
             whether any bits are set.
         **/
        operator bool() const {
            return reg > 0;
        }
    };

    /**
        MSB-first or LSB-first data order.
    **/
    typedef enum {
        /// Most significant bit first
        COM_MSB_FIRST = 0,
        /// Least significant bit first
        COM_LSB_FIRST = 1
    } DataOrders;

    /**
        Sets the serial port into Asynchronous vs SPI mode.
    **/
    typedef enum {
        /// Universal Asynchronous Receiver-Transmitter (UART) mode
        UART,
        /// Serial Peripheral Interface (SPI) mode
        SPI
    } PortModes;

    /**
        The different polarities for the SPI clock.
    **/
    typedef enum {
        /// SCK is low when idle
        SCK_LOW = 0,
        /// SCK is high when idle
        SCK_HIGH
    } SpiClockPolarities;

    /**
        The SPI clock phase settings.
    **/
    typedef enum {
        /// Leading edge samples, trailing edge changes
        LEAD_SAMPLE = 0,
        /// Leading edge changes, trailing edge samples
        LEAD_CHANGE
    } SpiClockPhases;

    /**
        Modes for the serial port control lines.
    **/
    typedef enum {
        /// Control line is in the OFF state
        LINE_OFF,
        /// Control line is in the ON state
        LINE_ON,
        /// Control line is controlled by hardware
        LINE_HW,
    } CtrlLineModes;

    // ======================= MODE INDEPENDENT API ============================
    /**
        \brief Setup the port mode.

        Puts the port into SPI or UART mode for subsequent transfers.

        \code{.cpp}
        if (ConnectorCOM1.PortMode(SerialBase::UART)) {
            // Setting COM-1's mode to UART was successful
        }
        \endcode

        \param[in] newMode The requested serial mode.
        \return Returns true if the mode is successfully set.
    **/
    bool PortMode(PortModes newMode);

    // ========================== ISerial  API =================================
    /**
        \copydoc ISerial::Flush()
    **/
    void Flush() override;
    /**
        \copydoc ISerial::FlushInput()
    **/
    void FlushInput() override;

    /**
       \copydoc ISerial::PortOpen()
    **/
    virtual void PortOpen() override;

    /**
        \copydoc ISerial::PortClose()
    **/
    virtual void PortClose() override;

    /**
        \copydoc ISerial::Speed(uint32_t bitsPerSecond)
        Will return false if the baud rate gets clipped for SPI mode.
    **/
    virtual bool Speed(uint32_t bitsPerSecond) override;

    /**
        \copydoc ISerial::Speed()
    **/
    virtual uint32_t Speed() override {
        return m_baudRate;
    }

    /**
        \copydoc ISerial::CharGet()
    **/
    int16_t CharGet() override;

    /**
        \copydoc ISerial::CharPeek()
    **/
    int16_t CharPeek() override;

    /**
        \copydoc ISerial::SendChar(uint8_t charToSend)
    **/
    bool SendChar(uint8_t charToSend) override;

    /**
        \copydoc ISerial::AvailableForRead()
    **/
    int32_t AvailableForRead() override;

    /**
         \copydoc ISerial::AvailableForWrite()
    **/
    int32_t AvailableForWrite() override;

    /**
        \copydoc ISerial::WaitForTransmitIdle()
    **/
    void WaitForTransmitIdle() override;

    /**
        \copydoc ISerial::PortIsOpen()
    **/
    bool PortIsOpen() override;

    /**
        \brief Set UART CTS/RTS flow control.

        \code{.cpp}
        ConnectorCOM0.FlowControl(true);
        \endcode

        \param[in] useFlowControl The new flow control setting.
        \note Flow control is off by default. Some XBee devices have flow
        control enabled by default. If using an XBee device, the ClearCore flow
        control setting should match the XBee device setting.
    **/
    void FlowControl(bool useFlowControl);

    /**
        \brief Return whether UART CTS/RTS flow control is enabled.

        \code{.cpp}
        if (ConnectorCOM0.FlowControl()) {
            // COM-0's flow control is enabled
        }
        \endcode

        \return Returns true if port flow control setting is enabled.
    **/
    bool FlowControl() {
        return m_flowControl;
    };

    /**
        \brief Change the serial RTS mode

        \code{.cpp}
        if (ConnectorCOM0.RtsMode(SerialBase::LINE_ON)) {
            // COM-0's RTS line has asserted successfully
        }
        \endcode

        \param[in] mode The desired RTS pin mode.
        \return Returns true if the mode was set.
        \note Using #LINE_HW with #FlowControl enabled will
        assert RTS when the Serial Port is ready to receive data.
    **/
    bool RtsMode(CtrlLineModes mode);

    /**
        \brief Read the serial CTS state.

        \code{.cpp}
        if (ConnectorCOM0.CtsState()) {
            // COM-0's CTS pin is logical LINE_ON.
        }
        \endcode

        \return Returns true if the CTS pin is #LINE_ON.
    **/
    bool CtsState();

    /**
        \brief Initiate a Serial Break

        \code{.cpp}
        // Enables a serial break on COM-0
        ConnectorCOM1.SerialBreak(true);
        \endcode

        \param[in] enable true enables a serial break, false disables a serial
        break
    **/
    void SerialBreak(bool enable);

    /**
        \brief Set UART transmission parity format.

        \code{.cpp}
        if (ConnectorCOM1.Parity(SerialBase::PARITY_O)) {
            // Setting COM-1's parity format to odd was successful
        }
        \endcode

        \return Returns true if port accepted the format change request.
    **/
    bool Parity(Parities newParity) override;

    /**
        \brief Return current port UART transmission format.

        \code{.cpp}
        if (ConnectorCOM0.Parity() == SerialBase::PARITY_O) {
            // COM-0's parity format is set to odd
        }
        \endcode

        \return Returns transmission format enumeration.
    **/
    Parities Parity() override {
        return m_parity;
    }

    /**
        Change the number of stop bits used in UART communication.

        \code{.cpp}
        // Sets COM-0 to require two stop bits
        ConnectorCOM1.StopBits(2);
        \endcode
    **/
    bool StopBits(uint8_t bits) override;

    /**
        \brief Change the number of bits in a character.

        For UART mode valid settings are: 5, 6, 7, 8, or 9.
        For SPI mode valid settings are: 8 or 9.

        \code{.cpp}
        // Sets COM-0 to have a 9-bit character size
        ConnectorCOM1.CharSize(9);
        \endcode
    **/
    bool CharSize(uint8_t size) override;

#ifndef HIDE_FROM_DOXYGEN
    /**
        bool operator for compatibility with ISerial
    **/
    operator bool() override {
        return true;
    }
#endif

    // =============================== SPI API =================================

    /**
        \brief Change the polarity and phase for the SPI clock

        \code{.cpp}
        // Set up COM-0's SPI clock
        ConnectorCOM0.SpiClock(SerialBase::SCK_HIGH, SerialBase::LEAD_SAMPLE);
        \endcode
    **/
    void SpiClock(SpiClockPolarities polarity, SpiClockPhases phase);

    /**
        \brief Change the SPI Slave Select mode

        \code{.cpp}
        if (ConnectorCOM0.SpiSsMode(SerialBase::LINE_HIGH)) {
            // COM-0's SPI slave select mode set to high successfully
        }
        \endcode

        \param[in] mode The desired SS pin mode.
        \return Returns true if the mode was set.
    **/
    bool SpiSsMode(CtrlLineModes mode);

    /**
        SPI's transmit and receive function
    **/
    uint8_t SpiTransferData(uint8_t data);

    /**
        \brief SPI's multi-byte transmit and receive function

        This can be used to send/receive a buffer's worth of data. The
        SPI channel will be commanded to transfer a byte at a time for
        the given \a len of bytes. The data transferred out will come from
        the \a writeBuf or a dummy value, and the data received in will be
        written to the \a readBuf or a dummy value.

        \param[in] writeBuf A pointer to the beginning of the data to write
        \param[in] readBuf A pointer to the beginning of a read destination
        for the data in the serial port
        \param[in] len The maximum number of bytes to read and write
        \return The number of bytes written or read.
    **/
    int32_t SpiTransferData(
        uint8_t const *writeBuf, uint8_t *readBuf, int32_t len);

    /**
        \brief SPI's asynchronous multi-byte transmit and receive function

        This can be used to send/receive a buffer's worth of data. The
        SPI channel will be commanded to transfer \a len of bytes asynchronously
        via the Direct Memory Access Controller. The data transferred out will
        come from the \a writeBuf or a dummy value, and the data received in
        will be written to the \a readBuf or a dummy value.

        \param[in] writeBuf A pointer to the beginning of the data to write
        \param[in] readBuf A pointer to the beginning of a read destination
        for the data in the serial port
        \param[in] len The maximum number of bytes to read and write
        \return True if transfer completed successfully, false otherwise.
    **/
    bool SpiTransferDataAsync(
        uint8_t const *writeBuf, uint8_t *readBuf, int32_t len);

    /**
        \brief Block until asynchronous transfers are completed.
        \return True when all asynchronous transfers are completed.
        Does not return false.
    **/
    bool SpiAsyncWaitComplete();

    // ============================= SETUP API =================================

    /**
        \brief Change the data order for the port.

        For UART, this should most likely be set to #COM_LSB_FIRST.
    **/
    void DataOrder(DataOrders newOrder);

    // ========================= ERROR HANDLING API ============================
    /**
        Handles frame errors by clearing the error flag and raising an
        internal warning flag.
    **/
    void HandleFrameError();

    /**
        Handles parity errors by clearing the error flag and raising an
        internal warning flag.
    **/
    void HandleParityError();

    /**
        Handles overflow errors by clearing the error flag and raising an
        internal warning flag.
    **/
    void HandleOverflow();

    /**
        \brief Accumulating clear on read accessor for any error status bits
        that were asserted sometime since the previous invocation of this
        function.

        \code{.cpp}
        // See if an overflow error has occurred on COM-0 since the last poll.
        bool serialOverflowError =
            ConnectorCOM0.ErrorStatusAccum().bit.SerialOverflowError;
        if (serialOverflowError) {
            // Deal with the overflow.
        }
        \endcode

        \param[in] mask (optional) A SerialErrorStatusRegister whose asserted
        bits indicate which of the error status bits to check for an asserted
        state. If one of the \a bit members of this mask are deasserted, that
        bit will be ignored when checking for asserted status bits. If no
        \a mask is provided, it's equivalent to passing a
        SerialErrorStatusRegister with all bits asserted, in which case this
        function would report any accumulated asserted error status bits.

        \return SerialErrorStatusRegister whose asserted bits indicate which
        serial error status bits have been asserted since the last poll.
    **/
    SerialErrorStatusRegister ErrorStatusAccum(SerialErrorStatusRegister mask =
                UINT32_MAX);

#ifndef HIDE_FROM_DOXYGEN
    //========================= INTERRUPT HANDLERS =============================

    /**
        \brief Should be called by SERCOMx_0 Interrupt Vector.

        This is typically associated with the transmit (TX) data service.
    **/
    void IrqHandlerTx();
    /**
        Should be called by SERCOMx_1 Interrupt Vector.
    **/
    void IrqHandler1();
    /**
        \brief Should be called by SERCOMx_2 Interrupt Vector.

        This is typically associated with RX data service.
    **/
    void IrqHandlerRx();
    /**
        \brief Should be called by SERCOMx_3 Interrupt Vector.

        This is typically called on port exceptions.
    **/
    void IrqHandlerException();
#endif

protected:
    // Current format
    Parities m_parity;
    uint8_t m_stopBits;
    uint8_t m_charSize;
    PortModes m_portMode;
    SpiClockPolarities m_polarity;
    SpiClockPhases m_phase;
    CtrlLineModes m_ssMode;
    CtrlLineModes m_rtsMode;
    bool m_flowControl;

    // SERCOM instance
    Sercom *m_serPort;

    // Pin information
    const PeripheralRoute *m_ctsMisoInfo;
    const PeripheralRoute *m_rtsSsInfo;
    const PeripheralRoute *m_rxSckInfo;
    const PeripheralRoute *m_txMosiInfo;

    uint32_t m_baudRate;
    uint8_t m_peripheral;
    // Port open/close state
    bool m_portOpen;
    // Serial Break state
    bool m_serialBreak;
    // SERCOM DRE interrupt number
    IRQn_Type m_dreIrqN;
    // SPI dma channels
    DmaChannels m_dmaRxChannel;
    DmaChannels m_dmaTxChannel;

    /**
        Construct and wire this serial port into the PADs.
    **/
    SerialBase(const PeripheralRoute *ctsMisoInfo,
               const PeripheralRoute *rtsSsInfo,
               const PeripheralRoute *rxSckInfo,
               const PeripheralRoute *txMosiInfo,
               uint8_t peripheral);

    /**
        Default constructor so this connector can be a global and constructed
        by SysManager
    **/
    SerialBase() {};

    /**
        Delay function to let the line state settle out.
    **/
    void WaitOneCharTime();

    // =========================== INTERRUPT API ===============================
    /**
        Enable the Data Register Empty UART interrupt.
    **/
    void EnableDreInterruptUart();
    /**
        Disable the Data Register Empty UART interrupt.
    **/
    void DisableDreInterruptUart();
    /**
        Enable the Receive Complete UART interrupt.
    **/
    void EnableRxcInterruptUart();
    /**
        Disable the Receive Complete UART interrupt.
    **/
    void DisableRxcInterruptUart();

private:
    // Serial Buffers
    int16_t m_bufferIn[SERIAL_BUFFER_SIZE];
    int16_t m_bufferOut[SERIAL_BUFFER_SIZE];
    // Indices for head and tails of the ring buffers

    volatile uint32_t m_inHead, m_inTail;
    volatile uint32_t m_outHead, m_outTail;

    // Clear-on-read accumulating error register.
    SerialErrorStatusRegister m_errorRegAccum;

    /**
        Enables and disables the SPI connector and waits for the enable
        status to sync properly.
    **/
    void PortEnable(bool initializing = false);
    void PortDisable();

    /**
        Helper function to get next index in a buffer.
    **/
    uint32_t NextIndex(uint32_t currentIndex) {
        return ((currentIndex + 1) & (SERIAL_BUFFER_SIZE - 1));
    }

    /**
        Receives characters from the DATA register and places them in the
        receiving buffer.
    **/
    void RxProc();

    /**
        Transmit any data waiting in the transmit buffer.
    **/
    void TxPump();

    /**
        Helper function for setting RTS/SS pin modes
    **/
    bool RtsSsPinState(CtrlLineModes mode);
};

} // ClearCore namespace
#endif // __SERIALBASE_H__
