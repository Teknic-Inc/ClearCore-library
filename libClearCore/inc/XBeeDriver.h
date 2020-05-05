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
    \file XBeeDriver.h
    This class controls access to the XBee wireless controller.

    It will allow you to setup:
        - Achieve a wireless serial port to a remote location
**/

#ifndef __XBEEDRIVER_H__
#define __XBEEDRIVER_H__

#include <stdint.h>
#include "PeripheralRoute.h"
#include "SerialDriver.h"

namespace ClearCore {

/**
    \brief ClearCore XBee Interface

    This class manages access to the XBee module on the ClearCore board.
**/
class XBeeDriver : public SerialBase {
    friend class SysManager;

public:

#ifndef HIDE_FROM_DOXYGEN
    /**
        Default constructor so this connector can be a global and constructed
        by SysManager.
    **/
    XBeeDriver() {};
#endif

private:
    /**
        Constructor, wires in pins and non-volatile info.
    **/
    XBeeDriver(const PeripheralRoute *ctsInfo, const PeripheralRoute *rtsInfo,
               const PeripheralRoute *rxInfo, const PeripheralRoute *txInfo,
               uint8_t peripheral);

};

} // ClearCore namespace
#endif // __XBEEDRIVER_H__