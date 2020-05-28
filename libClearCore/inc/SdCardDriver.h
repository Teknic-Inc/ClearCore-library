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
    \file SdCardDriver.h
    This class controls access to the micro SD Card reader.

    The class will provide SD card support for data logging, configuration
    files, and disk emulation.
**/

#ifndef __SDCARDDRIVER_H__
#define __SDCARDDRIVER_H__

#include <stdint.h>
#include "PeripheralRoute.h"
#include "SerialBase.h"

namespace ClearCore {

/**
    \brief ClearCore SD card interface

    This class manages access to the micro SD Card reader.
**/
class SdCardDriver : public SerialBase {
    friend class SysManager;

public:
#ifndef HIDE_FROM_DOXYGEN
    /**
        \brief Default constructor so this connector can be a global and
        constructed by SysManager
    **/
    SdCardDriver() {};

    /**
        \brief Signal an error in the SD card

        \param[in] errorCode An error code; constants are defined in SD.h
    **/
    void SetErrorCode(uint8_t errorCode) {
        m_errorCode = errorCode;
    }

    /**
        \brief Check if the SD card is in a fault state

        \return True if an error code is present
    **/
    bool IsInFault() {
        return (m_errorCode != 0);
    }
#endif // HIDE_FROM_DOXYGEN

private:
    uint8_t m_errorCode;

    /**
        Construction, wires in pins and non-volatile info.
    **/
    SdCardDriver(const PeripheralRoute *misoPin,
                 const PeripheralRoute *ssPin,
                 const PeripheralRoute *sckPin,
                 const PeripheralRoute *mosiPin,
                 uint8_t peripheral);
};

} // ClearCore namespace

#endif // __SDCARDDRIVER_H__