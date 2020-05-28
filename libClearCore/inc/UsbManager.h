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
    \file UsbManager.h

    USB manager class to handle USB 2.1 communication as a device.

    General Operation:
        The USB peripheral has a built in DMA separate from the main DMA. It
        uses this DMA to transfer data to RAM for software to read.

    Abbreviations:
        EP -> Endpoint
        DIR -> Direction
        PCK -> Packet
        CDC -> Communication Device Class
        ACM -> Abstract Control Model
**/

#ifndef __USBMANAGER_H__
#define __USBMANAGER_H__

#include <stdint.h>
#include <stdio.h>
#include <sam.h>

// extern "C" {
//     #include "cdcdf_acm.h"
// }

#ifdef __cplusplus
extern "C" {
#endif
#include "hpl_usb.h"
#include "cdcdf_acm.h"
#include "cdcdf_acm_desc.h"
#include "hal_usb_device.h"
#include "hri_mclk_e53.h"
#include "hri_gclk_e53.h"
#include "hal_gpio.h"
#include "hri_port_e53.h"
#ifdef __cplusplus
}
#endif
namespace ClearCore {

/** USB serial buffer size, in bytes. (64) **/
#ifndef USB_SERIAL_BUFFER_SIZE
#define USB_SERIAL_BUFFER_SIZE 64
#endif

// List of all UsbStatusReg items. Will be used to generate bitfield, enums, and
// masks. Ensures that the three are kept up to date with each other.
#define USB_STATUS_REG_LIST(Func)                                      \
    Func(UnhandledSetupReq)                                            \
    Func(UnhandledDescReq)                                             \
    Func(UnhandledStringReq)                                           \
    Func(UnhandledFeatureReq)                                          \
    Func(FailedStandardSetup)                                          \
    Func(FailedClassSetup)                                             \
    Func(FailedDescriptor)                                             \
    Func(FailedTransferIn)                                             \
    Func(FailedTransferOut)                                            \
    Func(TimeoutRead)                                                  \
    Func(TimeoutWrite)                                                 \
    Func(TimeoutSync)                                                  \
    Func(RamAccessError)                                               \
    Func(FrameNumberCrcError)                                          \
    Func(ReadBufferOverflow)                                           \


// Used to populate the bit fields in a union
#ifdef STRUCTIFY
#undef STRUCTIFY
#endif
#define STRUCTIFY(item) \
uint32_t item  : 1;

// Used to populate the items in a macro
#ifdef ENUMIFY
#undef ENUMIFY
#endif
#define ENUMIFY(item) \
    item,

// Used to generate masks from a previously defined enum.
#ifdef MASKIFY
#undef MASKIFY
#endif
#define MASKIFY(item) \
    const uint32_t   item##Mask = 1UL << item ;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))


#ifndef HIDE_FROM_DOXYGEN
/**
    Status Register
**/
union UsbStatusRegister {
    /**
        Broad access to the whole register
    **/
    uint32_t reg;
    /**
        Field access to the status register
    **/
    struct {
        USB_STATUS_REG_LIST(STRUCTIFY)
    } bit;
};
#endif

#ifndef HIDE_FROM_DOXYGEN
/**
    \brief USB Status register fields enum

    Provides index into Status Register Fields with names matching those in
    USB_STATUS_REG_LIST.
**/
typedef enum {
    USB_STATUS_REG_LIST(ENUMIFY)
    USB_STATUS_REG_FIELD_LAST_ITEM
} UsbStatusRegFields;

#define UNUSED(expr) (void)(expr)

#endif
/**
    Generates a mask for all of the items in the USB_STATUS_REG_LIST. Appends
    the text "Mask" to the item in the list.

    e.g. The mask for TimeoutRead is TimeoutReadMask
**/
USB_STATUS_REG_LIST(MASKIFY)


/**
    \brief USB manager Class

    Implements a CDC (Communications Device Class) USB device. The CDC uses
    an abstract control model (ACM) to emulate a serial port.

    Reading:
        Upon receiving data (interrupt based), the data is copied into a
        circular buffer. When the buffer is full, the receipt of data is
        acknowledged, but not copied into the buffer. Data is drained from the
        buffer via the Read function. To Query the number of available bytes
        call Available().

    Writing:
        Small transfers (less than a packet size) are copied into a buffer
        and sent in the background. Larger transfers are still sent in the
        background, but are not buffered. This means that the pointer to the
        data must remain valid during the sending procedure.

**/
class UsbManager {
    friend class SysManager;
public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        Public accessor for singleton instance
    **/
    static UsbManager &Instance();
#endif

    /**
        \brief Change the baud rate for the port.

        \param[in] bitsPerSecond The new speed setting
        \return Returns true if port accepted the speed request.
    **/
    bool Speed(uint32_t bitsPerSecond);

    /**
        \brief Get current port speed.

        \return Returns port speed in bits per second.
    **/
    uint32_t Speed();

    /**
    \brief Return whether or not the USB port is open

    \return True if the port is open, and false otherwise.
    **/
    bool PortIsOpen();

    void PortOpen();
    void PortClose();

    void FlushInput();

    void WaitForWriteFinish();

    /**
       \copydoc ISerial::AvailableForRead()
    **/
    int32_t AvailableForRead();

    /**
         \copydoc ISerial::AvailableForWrite()
    **/
    int32_t AvailableForWrite();

    /**
        \copydoc ISerial::CharGet()
    **/
    int16_t CharGet();

    /**
        \copydoc ISerial::CharPeek()
    **/
    int16_t CharPeek();

    /**
        \copydoc ISerial::SendChar(uint8_t charToSend)
    **/
    bool SendChar(uint8_t charToSend);

    /**
        \brief Returns whether USB is connected and operational.
    **/
    operator bool();

    const volatile usb_cdc_control_signal_t &LineState() {
        return m_lineState;
    }

    UsbManager();

private:

    void Refresh();

    /**
        Receives characters from the DATA register and places them in the
        receiving buffer.
    **/
    void RxProc();

    /**
        Transmit any data waiting in the transmit buffer.
    **/
    void TxPump();
    static bool CBLineStateChanged(usb_cdc_control_signal_t state);
    static bool TxComplete(const uint8_t ep,
                           const enum usb_xfer_code rc,
                           const uint32_t count);
    static bool RxComplete(const uint8_t ep,
                           const enum usb_xfer_code rc,
                           const uint32_t count);

    void cdc_device_acm_init(void);


    // Serial Buffers
    __attribute__((__aligned__(4))) uint8_t m_bufferIn[USB_SERIAL_BUFFER_SIZE];
    __attribute__((__aligned__(4))) uint8_t m_bufferOut[USB_SERIAL_BUFFER_SIZE];

    __attribute__((__aligned__(4))) uint8_t m_usbReadBuf[USB_SERIAL_BUFFER_SIZE];
    __attribute__((__aligned__(4))) uint8_t m_usbWriteBuf[USB_SERIAL_BUFFER_SIZE];
    // Indices for head and tails of the ring buffers
    volatile uint32_t m_inHead, m_inTail;
    volatile uint32_t m_outHead, m_outTail;

    volatile bool m_sendActive;
    volatile bool m_readActive;
    usb_cdc_control_signal_t m_lineState;
    uint8_t *m_readBufPtr;
    uint32_t m_readBufAvail;

    bool m_portOpen;

    /**
        Initializes the UsbManager.
    **/
    bool Initialize();
    /**
        Software resets the USB peripheral. Initialization will have to
        be re-performed.
    **/
    void Reset();
    void RxCopyToRingBuf();
    /**
        \brief Returns whether USB is connected.
    **/
    bool Connected();

}; // UsbManager

} // ClearCore namespace
#endif //__USBMANAGER_H__
