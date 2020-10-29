/**
 * Copyright (c) 2011-2018 Bill Greiman
 * This file is part of the SdFat library for SD memory cards.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/**
 * \file
 * \brief SpiDriver classes
 */
#ifndef SdSpiDriver_h
#define SdSpiDriver_h
#include "SdSpiBaseDriver.h"
#include "stddef.h"
#include "SdCardDriver.h"
#include <stdint.h>
#include <sam.h>
//------------------------------------------------------------------------------
/**
 * \class SdSpiLibDriver
 * \brief SdSpiLibDriver - use standard SPI library.
 */

 namespace ClearCore {
     extern SdCardDriver SdCard;
 }

class SdSpiLibDriver {
public:

    typedef uint8_t pin_size_t;


    typedef enum {
        LOW = 0,
        HIGH = 1,
        CHANGE = 2,
        FALLING = 3,
        RISING = 4,
    } PinStatus;

    typedef enum {
        INPUT = 0x0,
        OUTPUT = 0x1,
        INPUT_PULLUP = 0x2,
    } PinMode;

    /** Activate SPI hardware. */
    void activate() {
        SdCard.SpiClock(SerialBase::SpiClockPolarities::SCK_LOW,
                        SerialBase::SpiClockPhases::LEAD_SAMPLE);
        SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_ON);
    }
    /** Deactivate SPI hardware. */
    void deactivate() {
        SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
    }
    /** Initialize the SPI bus.
     *
     * \param[in] csPin SD card chip select pin.
     */
    void begin(uint8_t csPin, uint32_t clockSpeed) {
        m_csPin = csPin;
        digitalWriteClearCore(csPin, (PinStatus)HIGH);
        pinModeClearCore(csPin, OUTPUT);
        SdCard.PortMode(SerialBase::PortModes::SPI);
        SdCard.SpiSsMode(SerialBase::CtrlLineModes::LINE_OFF);
        SdCard.Speed(clockSpeed);
        SdCard.SpiClock(SerialBase::SpiClockPolarities::SCK_LOW,
                        SerialBase::SpiClockPhases::LEAD_SAMPLE);
        SdCard.PortOpen();
    }
    /** Receive a byte.
     *
     * \return The byte.
     */
    uint8_t receive() {
        return SdCard.SpiTransferData(0XFF);
    }
    /** Receive multiple bytes.
    *
    * \param[out] buf Buffer to receive the data.
    * \param[in] n Number of bytes to receive.
    *
    * \return Zero for no error or nonzero error code.
    */
    uint8_t receive(uint8_t *buf, size_t n) {
        for (size_t i = 0; i < n; i++) {
            buf[i] = 0xFF;
        }
        const uint8_t *txbuff = buf;
        //blocking is disabled by default
        if (SdCard.SpiTransferDataAsync((uint8_t *)txbuff, (uint8_t *)buf, n)){
            //Make sure to update value before leaving
            SdCard.Refresh();
            while (!SdCard.getSDTransferComplete()) {
                //SPI transfer is blocked here
                Delay_ms(1);
                continue;
            }
        }
        else{
            SdCard.SpiTransferData((uint8_t *)txbuff, (uint8_t *)buf, n);
        }
        return 0;
    }
    /** Send a byte.
     *
     * \param[in] data Byte to send
     */
    void send(uint8_t data) {
        SdCard.SpiTransferData(data);
        return;
    }
    /** Send multiple bytes.
     *
     * \param[in] buf Buffer for data to be sent.
     * \param[in] n Number of bytes to send.
     */
    void send(const uint8_t *buf, size_t n) {
        if (SdCard.SpiTransferDataAsync((uint8_t *)buf, (uint8_t *)NULL, n)){
            //Make sure to update SD ISR before leaving
            SdCard.Refresh();
                while (!SdCard.getSDTransferComplete()) {
                    //SPI transfer is blocked here
                    continue;
                }
        }
        else{
            SdCard.SpiTransferData((uint8_t *)buf, (uint8_t *)NULL, n);
        }
    }
    /** Set CS low. */
    void select() {
        digitalWriteClearCore(m_csPin, (PinStatus)LOW);
    }
    /** Set CS high. */
    void unselect() {
        digitalWriteClearCore(m_csPin, (PinStatus)HIGH);

    }
    bool getSDTransferComplete(){
        return SdCard.getSDTransferComplete();
    }
    void setSDErrorCode(uint16_t errorCode){
        SdCard.SetErrorCode(errorCode);
    }

    /** Set SPI port.
     * \param[in] spiPort Hardware SPI port.
     */
//   void setPort(SPIClass* spiPort) {
//     m_spi = spiPort ? spiPort : &SDCARD_SPI;
//   }
private:
    uint8_t m_csPin;

    void digitalWriteClearCore(pin_size_t conNum, PinStatus ulVal) {
        // Get a reference to the appropriate connector
        ClearCore::Connector *connector =
        ClearCore::SysMgr.ConnectorByIndex(
        static_cast<ClearCorePins>(conNum));

        // If connector cannot be written, just return
        if (!connector || !connector->IsWritable()) {
            return;
        }

        connector->Mode(ClearCore::Connector::OUTPUT_DIGITAL);
        if (connector->Mode() == ClearCore::Connector::OUTPUT_DIGITAL) {
            connector->State(ulVal);
        }
    }

    void pinModeClearCore(pin_size_t pinNumber, uint32_t ulMode) {
        pinNumber = (ClearCorePins)pinNumber;
        ulMode = (PinMode)ulMode;
        // Get a reference to the appropriate connector
        ClearCore::Connector *connector =
        ClearCore::SysMgr.ConnectorByIndex(
        static_cast<ClearCorePins>(pinNumber));

        if (!connector) {
            return;
        }

        switch (ulMode) {
            case OUTPUT:
            connector->Mode(ClearCore::Connector::OUTPUT_DIGITAL);
            break;
            case INPUT:
            connector->Mode(ClearCore::Connector::INPUT_DIGITAL);
            break;
            case INPUT_PULLUP:
            connector->Mode(ClearCore::Connector::INPUT_DIGITAL);
            break;
            default:
            break;
        }
    }
};
//------------------------------------------------------------------------------
// Choose SPI driver for SdFat and SdFatEX classes.
/** SdFat uses Arduino library SPI. */
typedef SdSpiLibDriver SdFatSpiDriver;

// Don't need virtual driver.
typedef SdFatSpiDriver SdSpiDriver;

//==============================================================================
// Use of in-line for AVR to save flash.
#endif  // SdSpiDriver_h
