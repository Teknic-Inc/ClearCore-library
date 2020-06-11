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

#include "UsbManager.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sam.h>
#include <component/usb.h>
#include <component/mclk.h>
#include "HardwareMapping.h"
#include "ShiftRegister.h"
#include "SysManager.h"
#include "SysUtils.h"
#include "SysTiming.h"
#include "sam.h"


#ifdef __cplusplus
extern "C" {
#endif
#include "cdcdf_acm.h"
#include "cdcdf_acm_desc.h"
#include "hal_usb_device.h"
#ifdef __cplusplus
}
#endif

#define USB_INTERRUPT_PRIORITY    4

namespace ClearCore {

#define PA24 GPIO(GPIO_PORTA, 24)
#define PA25 GPIO(GPIO_PORTA, 25)

extern SysManager SysMgr;
UsbManager &UsbMgr = UsbManager::Instance();

#if CONF_USBD_HS_SP
static uint8_t single_desc_bytes[] = {
    // Device descriptors and Configuration descriptors list.
    CDCD_ACM_HS_DESCES_LS_FS
};
static uint8_t single_desc_bytes_hs[] = {
    // Device descriptors and Configuration descriptors list.
    CDCD_ACM_HS_DESCES_HS
};
#define CDCD_ECHO_BUF_SIZ CONF_USB_CDCD_ACM_DATA_BULKIN_MAXPKSZ_HS
#else
static uint8_t single_desc_bytes[] = {
    // Device descriptors and Configuration descriptors list.
    CDCD_ACM_DESCES_LS_FS
};
#define CDCD_ECHO_BUF_SIZ CONF_USB_CDCD_ACM_DATA_BULKIN_MAXPKSZ
#endif

static struct usbd_descriptors single_desc[]
    = {{single_desc_bytes, single_desc_bytes + sizeof(single_desc_bytes)}
#if CONF_USBD_HS_SP
    ,
    {
        single_desc_bytes_hs,
        single_desc_bytes_hs + sizeof(single_desc_bytes_hs)
    }
#endif
};

// Ctrl endpoint buffer
static uint8_t ctrl_buffer[64];

static void USB_DEVICE_INSTANCE_PORT_init(void);

int writeNum(char *buf, uint32_t n, bool full) {
    int i = 0;
    int sh = 28;
    while (sh >= 0) {
        int d = (n >> sh) & 0xf;
        if (full || d || sh == 0 || i) {
            buf[i++] = d > 9 ? 'A' + d - 10 : '0' + d;
        }
        sh -= 4;
    }
    return i;
}

static void load_serial_number() {
    // Overwrite the serial number descriptor in memory (16-bit unicode)
    uint16_t *serNumDescPtr = (uint16_t *)usb_find_str_desc(single_desc_bytes,
                              single_desc_bytes + sizeof(single_desc_bytes),
                              CONF_USB_CDCD_ACM_ISERIALNUM);
    if (!serNumDescPtr) {
        // Descriptor does not exist, bail out
        return;
    }
    // Serial numbers are derived from four 32-bit words.
#define SERIAL_NUMBER_LENGTH (4 * 8)
    // serial_number will be filled in when needed.
    char serial_number[SERIAL_NUMBER_LENGTH];

    // These are locations that taken together make up a unique serial number.
    uint32_t *addresses[4] = {(uint32_t *) 0x008061FC, (uint32_t *) 0x00806010,
                              (uint32_t *) 0x00806014, (uint32_t *) 0x00806018
                             };
    uint32_t serial_number_idx = 0;
    for (int i = 0; i < 4; i++) {
        serial_number_idx += writeNum(&(serial_number[serial_number_idx]),
                                      *(addresses[i]), true);
    }

    // Skip over the length and type bytes
    serNumDescPtr++;
    for (int i = 0; i < SERIAL_NUMBER_LENGTH; i++) {
        *serNumDescPtr++ = serial_number[i];
    }
}

/**
    CDC ACM Init
**/
void UsbManager::cdc_device_acm_init(void) {
    // Usb stack init
    usbdc_init(ctrl_buffer);

    // usbdc_register_funcion inside
    cdcdf_acm_init();

    usbdc_start(single_desc);
    usbdc_attach();
}

void USB_DEVICE_INSTANCE_PORT_init(void) {
    gpio_set_pin_direction(PA24,
                           // <y> Pin direction
                           // <id> pad_direction
                           // <GPIO_DIRECTION_OFF"> Off
                           // <GPIO_DIRECTION_IN"> In
                           // <GPIO_DIRECTION_OUT"> Out
                           GPIO_DIRECTION_OUT);

    gpio_set_pin_level(PA24,
                       // <y> Initial level
                       // <id> pad_initial_level
                       // <false"> Low
                       // <true"> High
                       false);

    gpio_set_pin_pull_mode(PA24,
                           // <y> Pull configuration
                           // <id> pad_pull_config
                           // <GPIO_PULL_OFF"> Off
                           // <GPIO_PULL_UP"> Pull-up
                           // <GPIO_PULL_DOWN"> Pull-down
                           GPIO_PULL_OFF);

    gpio_set_pin_function(PA24,
                          // <y> Pin function
                          // <id> pad_function
                          // <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
                          // <PINMUX_PA24H_USB_DM"> Auto
                          // <GPIO_PIN_FUNCTION_OFF"> Off
                          // <GPIO_PIN_FUNCTION_A"> A
                          // <GPIO_PIN_FUNCTION_B"> B
                          // <GPIO_PIN_FUNCTION_C"> C
                          // <GPIO_PIN_FUNCTION_D"> D
                          // <GPIO_PIN_FUNCTION_E"> E
                          // <GPIO_PIN_FUNCTION_F"> F
                          // <GPIO_PIN_FUNCTION_G"> G
                          // <GPIO_PIN_FUNCTION_H"> H
                          // <GPIO_PIN_FUNCTION_I"> I
                          // <GPIO_PIN_FUNCTION_J"> J
                          // <GPIO_PIN_FUNCTION_K"> K
                          // <GPIO_PIN_FUNCTION_L"> L
                          // <GPIO_PIN_FUNCTION_M"> M
                          // <GPIO_PIN_FUNCTION_N"> N
                          PINMUX_PA24H_USB_DM);

    gpio_set_pin_direction(PA25,
                           // <y> Pin direction
                           // <id> pad_direction
                           // <GPIO_DIRECTION_OFF"> Off
                           // <GPIO_DIRECTION_IN"> In
                           // <GPIO_DIRECTION_OUT"> Out
                           GPIO_DIRECTION_OUT);

    gpio_set_pin_level(PA25,
                       // <y> Initial level
                       // <id> pad_initial_level
                       // <false"> Low
                       // <true"> High
                       false);

    gpio_set_pin_pull_mode(PA25,
                           // <y> Pull configuration
                           // <id> pad_pull_config
                           // <GPIO_PULL_OFF"> Off
                           // <GPIO_PULL_UP"> Pull-up
                           // <GPIO_PULL_DOWN"> Pull-down
                           GPIO_PULL_OFF);

    gpio_set_pin_function(PA25,
                          // <y> Pin function
                          // <id> pad_function
                          // <i> Auto : use driver pinmux if signal is imported by driver, else turn off function
                          // <PINMUX_PA25H_USB_DP"> Auto
                          // <GPIO_PIN_FUNCTION_OFF"> Off
                          // <GPIO_PIN_FUNCTION_A"> A
                          // <GPIO_PIN_FUNCTION_B"> B
                          // <GPIO_PIN_FUNCTION_C"> C
                          // <GPIO_PIN_FUNCTION_D"> D
                          // <GPIO_PIN_FUNCTION_E"> E
                          // <GPIO_PIN_FUNCTION_F"> F
                          // <GPIO_PIN_FUNCTION_G"> G
                          // <GPIO_PIN_FUNCTION_H"> H
                          // <GPIO_PIN_FUNCTION_I"> I
                          // <GPIO_PIN_FUNCTION_J"> J
                          // <GPIO_PIN_FUNCTION_K"> K
                          // <GPIO_PIN_FUNCTION_L"> L
                          // <GPIO_PIN_FUNCTION_M"> M
                          // <GPIO_PIN_FUNCTION_N"> N
                          PINMUX_PA25H_USB_DP);
}

UsbManager &UsbManager::Instance() {
    static UsbManager *instance = new UsbManager();
    return *instance;
}

UsbManager::UsbManager() :
    m_inHead(0),
    m_inTail(0),
    m_outHead(0),
    m_outTail(0),
    m_sendActive(false),
    m_readActive(false),
    m_readBufPtr(m_usbReadBuf),
    m_readBufAvail(0),
    m_portOpen(false) {
    m_lineState.value = 0;
    cdcdf_acm_register_callback(CDCDF_ACM_CB_STATE_C,
                                (FUNC_PTR)CBLineStateChanged);
}

bool UsbManager::Initialize() {
    // Enable 48MHz clock to USB module from GCLK4
    SET_CLOCK_SOURCE(USB_GCLK_ID, 4);
    CLOCK_ENABLE(AHBMASK, USB_);
    CLOCK_ENABLE(APBBMASK, USB_);

    load_serial_number();

    NVIC_SetPriority(USB_0_IRQn, USB_INTERRUPT_PRIORITY);
    NVIC_SetPriority(USB_1_IRQn, USB_INTERRUPT_PRIORITY);
    NVIC_SetPriority(USB_2_IRQn, USB_INTERRUPT_PRIORITY);
    NVIC_SetPriority(USB_3_IRQn, USB_INTERRUPT_PRIORITY);

    USB_DEVICE_INSTANCE_PORT_init();

    cdc_device_acm_init();

    return true;
}

bool UsbManager::Speed(uint32_t bitsPerSecond) {
    // Speed is not set via this API for USB serial ports
    (void)bitsPerSecond;
    return true;
}

uint32_t UsbManager::Speed() {
    return cdcdf_acm_get_line_coding()->dwDTERate;
}

/**
    Sets the line break bit in the line state, then notifies the Host.

    Currently not used, but it provides an example of how to notify the Host
    that something has happened. Most Hosts ignore most notifications, but
    if needed, this should be the proper way to send a notification.
**/

/**
    Callback invoked when Line State Change
**/
bool UsbManager::CBLineStateChanged(usb_cdc_control_signal_t state) {
    UsbMgr.m_lineState = state;
    if (state.rs232.DTR) {
        // Callbacks must be registered after endpoint allocation
        cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)RxComplete);
        cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)TxComplete);
        // Start Rx
        cdcdf_acm_read(UsbMgr.m_usbReadBuf, sizeof(UsbMgr.m_usbReadBuf));
    }
    else {
        // Callbacks must be registered after endpoint allocation
        cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)NULL);
        cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)NULL);
        // Stop Rx/Tx
        cdcdf_acm_stop_xfer();
        if (cdcdf_acm_get_line_coding()->dwDTERate == 1200) {
            SysMgr.ResetBoard(SysManager::RESET_TO_BOOTLOADER);
        }
    }

    // No error
    return false;
}

bool UsbManager::PortIsOpen() {
    return (bool) * this && m_portOpen;
}

void UsbManager::PortOpen() {
    if (m_portOpen) {
        return;
    }

    m_portOpen = true;

    // Callbacks must be registered after endpoint allocation
    cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)RxComplete);
    cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)TxComplete);
    // Start Rx
    cdcdf_acm_read(m_usbReadBuf, sizeof(m_usbReadBuf));
}

void UsbManager::PortClose() {
    if (!m_portOpen) {
        return;
    }

    // Flush the transmit buffer before closing
    TxPump();
    WaitForWriteFinish();

    m_portOpen = false;

    // Callbacks must be registered after endpoint allocation
    cdcdf_acm_register_callback(CDCDF_ACM_CB_READ, (FUNC_PTR)NULL);
    cdcdf_acm_register_callback(CDCDF_ACM_CB_WRITE, (FUNC_PTR)NULL);
    // Stop Rx/Tx
    cdcdf_acm_stop_xfer();

    m_inHead = 0;
    m_inTail = 0;
    m_outHead = 0;
    m_outTail = 0;
    m_readBufAvail = 0;
    m_sendActive = false;
    m_readActive = false;
}

void UsbManager::FlushInput() {
    m_inHead = 0;
    m_inTail = 0;
    m_readActive = false;
    m_readBufAvail = 0;
    cdcdf_acm_read(m_usbReadBuf, sizeof(m_usbReadBuf));
}

void UsbManager::WaitForWriteFinish() {
    while (m_outHead != m_outTail && Connected()) {
        continue;
    }
}

bool UsbManager::Connected() {
    return cdcdf_acm_is_enabled() && LineState().rs232.DTR &&
           USB->DEVICE.FSMSTATUS.bit.FSMSTATE == USB_FSMSTATUS_FSMSTATE_ON;
}

UsbManager::operator bool() {
    bool retVal = Connected();
    Delay_ms(10);
    return retVal;
}

int16_t UsbManager::CharGet() {
    uint32_t head = m_inHead;
    if (m_inTail == head) {
        return -1;
    }
    uint8_t retVal = m_bufferIn[head];
    m_inHead = (head + 1) & (sizeof(m_bufferIn) - 1);
    RxCopyToRingBuf();
    return retVal;
}

int16_t UsbManager::CharPeek() {
    if (m_inTail == m_inHead) {
        return -1;
    }
    return m_bufferIn[m_inHead];
}

bool UsbManager::SendChar(uint8_t charToSend) {
    while (Connected() && m_portOpen) {
        if (AvailableForWrite()) {
            m_bufferOut[m_outTail] = charToSend;
            m_outTail = (m_outTail + 1) & (sizeof(m_bufferOut) - 1);
            return true;
        }
    }
    return false;
}

/**
    Return the number of free characters in the receive buffer
**/
int32_t UsbManager::AvailableForRead() {
    int32_t difference = m_inTail - m_inHead;

    if (difference < 0) {
        return sizeof(m_bufferIn) + difference;
    }
    else {
        return difference;
    }
}

/**
    Returns the number of available characters in the transmit buffer
**/
int32_t UsbManager::AvailableForWrite() {
    int32_t difference = m_outHead - m_outTail - 1;

    if (difference < 0) {
        difference += sizeof(m_bufferOut);
    }

    return difference;
}

/**
    Transmit any data waiting in the tx buffer
**/
void UsbManager::TxPump() {
    if (atomic_test_and_set_acqrel(&m_sendActive)) {
        // Already sending; can't send anything more right now
        return;
    }

    uint32_t head = m_outHead;
    uint32_t tail = m_outTail;
    if (head == tail) {
        // Nothing to send, bail out
        atomic_clear_seqcst(&m_sendActive);
        return;
    }

    // The data sent to cdcdf_acm_write needs to be 4-byte aligned,
    // so copy the data into an aligned buffer before sending it out
    uint32_t count = sizeof(m_usbWriteBuf);
    uint8_t *inPtr = m_bufferOut + head, *outPtr = m_usbWriteBuf;
    if (head < tail) {
        count = min(count, tail - head);
        for (uint32_t iChar = 0; iChar < count; iChar++) {
            *outPtr++ = *inPtr++;
        }
    }
    else {
        uint32_t countTilWrap = sizeof(m_bufferOut) - head;
        count = min(count, countTilWrap + tail);
        uint32_t loopEnd = min(countTilWrap, count);
        for (uint32_t iChar = 0; iChar < loopEnd; iChar++) {
            *outPtr++ = *inPtr++;
        }
        inPtr = m_bufferOut;
        loopEnd = count - loopEnd;
        for (uint32_t iChar = 0; iChar < loopEnd; iChar++) {
            *outPtr++ = *inPtr++;
        }
    }
    if (cdcdf_acm_write(m_usbWriteBuf, count)) {
        // cdcdf_acm_write failed, clear the send active flag
        atomic_clear_seqcst(&m_sendActive);
    }
}

bool UsbManager::TxComplete(const uint8_t ep,
                            const enum usb_xfer_code rc,
                            const uint32_t count) {
    UNUSED(ep);

    if (rc == USB_XFER_DONE) {
        UsbMgr.m_outHead =
            (UsbMgr.m_outHead + count) & (sizeof(m_bufferOut) - 1);
    }
    atomic_clear_seqcst(&UsbMgr.m_sendActive);
    UsbMgr.TxPump();

    return true;
}

bool UsbManager::RxComplete(const uint8_t ep,
                            const enum usb_xfer_code rc,
                            const uint32_t count) {
    UNUSED(ep);
    UNUSED(rc);

    __disable_irq();
    // Make the Rx data available to be copied into the Rx ring buffer
    UsbMgr.m_readBufAvail = count;
    UsbMgr.m_readBufPtr = UsbMgr.m_usbReadBuf;
    __enable_irq();
    UsbMgr.RxCopyToRingBuf();
    return true;
}
void UsbManager::Refresh(void) {
    if (!m_sendActive && m_outHead != m_outTail) {
        TxPump();
    }
}

void UsbManager::RxCopyToRingBuf() {
    __disable_irq();
    uint32_t space = sizeof(m_bufferIn) - 1 - AvailableForRead();
    if (m_readBufAvail && space) {

        uint32_t count = min(space, m_readBufAvail);
        uint32_t tail = m_inTail;
        uint8_t *inPtr = m_readBufPtr;
        uint8_t *outPtr = &m_bufferIn[tail];
        uint32_t countTilWrap = sizeof(m_bufferIn) - tail;
        countTilWrap = min(countTilWrap, count);

        // Copy the available data until we get to the
        // end of input data or the ring buffer wrap point
        for (uint32_t i = 0; i < countTilWrap; i++) {
            *outPtr++ = *inPtr++;
        }
        count -= countTilWrap;
        m_readBufPtr += countTilWrap;
        m_readBufAvail -= countTilWrap;
        if (count) {
            // Account for the ring buffer wrap point
            // and copy the remaining available data
            outPtr = m_bufferIn;
            for (uint32_t i = 0; i < count; i++) {
                *outPtr++ = *inPtr++;
            }
            m_inTail = count;
            m_readBufPtr += count;
            m_readBufAvail -= count;
        }
        else {
            m_inTail = (tail + countTilWrap) & (sizeof(m_bufferIn) - 1);
        }

        // If all of the available input data has been copied into the
        // ring buffer, read more input data from the USB device
        if (!m_readBufAvail) {
            cdcdf_acm_read(m_usbReadBuf, sizeof(m_usbReadBuf));
        }
    }
    __enable_irq();
}

} // ClearCore namespace
