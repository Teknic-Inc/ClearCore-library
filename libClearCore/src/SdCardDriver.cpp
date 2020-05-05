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
    SD card interface implementation

    This class manages the optional micro SD card reader attached to the Teknic
    ClearCore.
**/

#include "SdCardDriver.h"
#include <sam.h>
#include "SysUtils.h"

namespace ClearCore {

/**
    Construct and wire into the board.
**/
SdCardDriver::SdCardDriver(const PeripheralRoute *misoInfo,
                           const PeripheralRoute *ssInfo,
                           const PeripheralRoute *sckInfo,
                           const PeripheralRoute *mosiInfo,
                           uint8_t peripheral)
    : SerialBase(misoInfo, ssInfo, sckInfo, mosiInfo, peripheral),
      m_errorCode(0) {
    PortMode(SerialBase::SPI);
    SpiClock(SCK_LOW, LEAD_SAMPLE);
    PortOpen();
}

} // ClearCore namespace