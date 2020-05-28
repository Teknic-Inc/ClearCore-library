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
    ClearCore USB Serial Connector Implementation.
**/
#include "SerialUsb.h"
#include "UsbManager.h"

namespace ClearCore {

extern UsbManager &UsbMgr;

SerialUsb::SerialUsb(uint16_t index) :
    m_index(index) {}

void SerialUsb::FlushInput() {
    UsbMgr.FlushInput();
}

void SerialUsb::Flush() {
    UsbMgr.WaitForWriteFinish();
}

bool SerialUsb::PortIsOpen() {
    return static_cast<bool>(UsbMgr);
}

void SerialUsb::PortOpen() {
    UsbMgr.PortOpen();
}

void SerialUsb::PortClose() {
    UsbMgr.PortClose();
}

bool SerialUsb::Speed(uint32_t bitsPerSecond) {
    return UsbMgr.Speed(bitsPerSecond);
}

uint32_t SerialUsb::Speed() {
    return UsbMgr.Speed();
}

int16_t SerialUsb::CharGet() {
    return UsbMgr.CharGet();
}

int16_t SerialUsb::CharPeek() {
    return UsbMgr.CharPeek();
}

bool SerialUsb::SendChar(uint8_t charToSend) {
    return UsbMgr.SendChar(charToSend);
}

int32_t SerialUsb::AvailableForRead() {
    return UsbMgr.AvailableForRead();
}

int32_t SerialUsb::AvailableForWrite() {
    return UsbMgr.AvailableForWrite();
}

void SerialUsb::WaitForTransmitIdle() {
    UsbMgr.WaitForWriteFinish();
}

SerialUsb::operator bool() {
    return static_cast<bool>(UsbMgr);
}

} // ClearCore namespace
