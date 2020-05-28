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
  \file PeripheralRoute.h
  \brief Defines the Peripheral Route structure, used in HardwareMapping
**/

#ifndef __PERIPHERAL_ROUTE_H__
#define __PERIPHERAL_ROUTE_H__

#include <sam.h>

/** ClearCore I/O pin groups **/
typedef enum {
    NOT_A_PORT = -1,
    PORTA = 0,
    PORTB,
    PORTC,
    CLEARCORE_PORT_MAX // Keep as last item
} ClearCorePorts;

#ifndef HIDE_FROM_DOXYGEN
struct PeripheralRoute {
    uint8_t pin_raw;
    ClearCorePorts gpioPort;
    uint8_t gpioPin;
    uint8_t extInt;
    bool extIntAvail;
    uint8_t adc0Channel;
    uint8_t adc1Channel;
    uint8_t dacChannel;
    uint8_t sercomNum;
    uint8_t sercomPadNum;
    uint8_t tcNum;
    uint8_t tcPadNum;
    uint8_t tccNum;
    uint8_t tccPadNum;
    uint8_t gclkNum;
};
#endif

#endif // __PERIPHERAL_ROUTE_H__
