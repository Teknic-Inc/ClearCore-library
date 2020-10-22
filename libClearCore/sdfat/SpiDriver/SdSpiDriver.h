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
#include "ClearCoreRef.h"
#include "stddef.h"
#include "SerialBase.h"
#include <stdint.h>
//------------------------------------------------------------------------------
/** SDCARD_SPI is defined if board has built-in SD card socket */
#ifndef SDCARD_SPI
#define SDCARD_SPI SPI
#endif  // SDCARD_SPI
//------------------------------------------------------------------------------
/**
 * \class SdSpiLibDriver
 * \brief SdSpiLibDriver - use standard SPI library.
 */

class SdSpiLibDriver {
 public:
  /** Activate SPI hardware. */
  void activate() {
    SDCARD_SPI.beginTransaction();
  }
  /** Deactivate SPI hardware. */
  void deactivate() {
    SDCARD_SPI.endTransaction();
  }
  /** Initialize the SPI bus.
   *
   * \param[in] csPin SD card chip select pin.
   */
  void begin(uint8_t csPin,uint32_t clockSpeed) {
    m_csPin = csPin;
	digitalWriteClearCore(csPin, (PinStatus)HIGH);
    pinModeClearCore(csPin, OUTPUT);
    SDCARD_SPI.begin(clockSpeed);
  }
  /** Receive a byte.
   *
   * \return The byte.
   */
  uint8_t receive() {
    return SDCARD_SPI.transfer( 0XFF);
  }
  /** Receive multiple bytes.
  *
  * \param[out] buf Buffer to receive the data.
  * \param[in] n Number of bytes to receive.
  *
  * \return Zero for no error or nonzero error code.
  */
  uint8_t receive(uint8_t* buf, size_t n) {
	for (size_t i = 0; i < n; i++) {
		buf[i] = 0xFF;
	}
    const uint8_t *txbuff = buf;
	//blocking is disabled by default
	SDCARD_SPI.transfer(txbuff,buf,n,DONT_WAIT_FOR_TRANSFER);
	//Make sure to update value before leaving
	SdCard.SDCardISR();
	while(!getSDTransferComplete()){
		//SPI transfer is blocked here
		continue;
	}

    return 0;
  }
  /** Send a byte.
   *
   * \param[in] data Byte to send
   */
  void send(uint8_t data) {
    SDCARD_SPI.transfer(data);
  }
  /** Send multiple bytes.
   *
   * \param[in] buf Buffer for data to be sent.
   * \param[in] n Number of bytes to send.
   */
  void send(const uint8_t* buf, size_t n) {
	   SDCARD_SPI.transfer(buf,NULL,n,DONT_WAIT_FOR_TRANSFER);
	    //Make sure to update SD ISR before leaving
		SdCard.SDCardISR();	
		while(!getSDTransferComplete()){
			//SPI transfer is blocked here
			continue;
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

  /** Set SPI port.
   * \param[in] spiPort Hardware SPI port.
   */
//   void setPort(SPIClass* spiPort) {
//     m_spi = spiPort ? spiPort : &SDCARD_SPI;
//   }
 private:
  CCSPI* m_spi;
  uint8_t m_csPin;
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
