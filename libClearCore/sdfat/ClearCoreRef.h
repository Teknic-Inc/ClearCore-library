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

#ifndef CLEARCOREREF_H_
#define CLEARCOREREF_H_

#include "ClearCore.h"
#include "SerialBase.h"

#define SDCARD_SPI SPI2

#define MAX_SPI 10000000
#define SPI_MIN_CLOCK_DIVIDER 1

//define transfer blocking parameters
#define WAIT_FOR_TRANSFER true
#define DONT_WAIT_FOR_TRANSFER false

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

/**
    \brief This function sends data to a Clear Core connector

    \param[in] conNum, specifies the pin connection number
    \param[in] ulVal, state being set of the specified connector
**/
void digitalWriteClearCore(pin_size_t conNum, PinStatus ulVal);

/**
        This function replaces pinModeAPI in most cases as ClearCore uses
        a connector model in place of the pin model of the traditional Arduino
        implementations.

        \param[in] pinNumber, the number of the pin being set
        \param[in] ulMode, the mode the specified pin is set to
    **/
void pinModeClearCore(pin_size_t pinNumber, uint32_t ulMode);

/**
    \brief This function updates the Clear Core status register if there is an error

    \param[in] errorCode An error code; 0 = no error, !0 = error
**/
void setSDErrorCode(uint16_t errorCode);

/**
    \brief This function returns false if a transfer is in progress, true if not
**/
bool getSDTransferComplete();

/**
    \brief Clear Core SPI connection class

    \param[in] &thePort, the adress of the SPI or serial port
    \param[in] isCom, specifies whether
**/
class CCSPI {
public:
    CCSPI(ClearCore::SerialBase &thePort, bool isCom);

    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    void transfer(void *buf, size_t count);
    void transfer(const void *txbuf, void *rxbuf, size_t count,
                  bool block = true);
    void waitForTransfer(void);

    // Transaction Functions
    void usingInterrupt(int interruptNumber);
    void notUsingInterrupt(int interruptNumber);
    void beginTransaction();
    void endTransaction(void);

    void begin(uint32_t clock);
    void end();

    void setDataMode(uint8_t uc_mode);
    void setClockDivider(uint8_t uc_div);
    void SetClockSpeed(uint32_t clockSpeed);

private:
    void config();

    ClearCore::SerialBase *m_serial;
    bool m_isCom;
    uint32_t m_clock;

};

extern CCSPI SPI;
extern CCSPI SPI1;
extern CCSPI SPI2;



#endif /* SDREF_H_ */