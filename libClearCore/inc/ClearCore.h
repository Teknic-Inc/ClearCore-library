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
 
#ifndef __CLEARCORE_H__
#define __CLEARCORE_H__

// Header files from the ClearCore hardware that define connectors available
#include "AdcManager.h"
#include "CcioBoardManager.h"
#include "DigitalIn.h"
#include "DigitalInAnalogIn.h"
#include "DigitalInOut.h"
#include "DigitalInOutAnalogOut.h"
#include "DigitalInOutHBridge.h"
#include "EthernetManager.h"
#include "InputManager.h"
#include "LedDriver.h"
#include "EncoderInput.h"
#include "MotorDriver.h"
#include "MotorManager.h"
#include "SdCardDriver.h"
#include "SerialDriver.h"
#include "SerialUsb.h"
#include "StatusManager.h"
#include "SysManager.h"
#include "SysTiming.h"
#include "XBeeDriver.h"


namespace ClearCore {

extern LedDriver ConnectorLed;              ///< User-driven LED instance

// IO connectors
extern DigitalInOutAnalogOut ConnectorIO0;  ///< IO-0 connector instance
extern DigitalInOut ConnectorIO1;           ///< IO-1 connector instance
extern DigitalInOut ConnectorIO2;           ///< IO-2 connector instance
extern DigitalInOut ConnectorIO3;           ///< IO-3 connector instance

// H-Bridge type connectors
extern DigitalInOutHBridge ConnectorIO4;    ///< IO-4 connector instance
extern DigitalInOutHBridge ConnectorIO5;    ///< IO-5 connector instance

// Digital input only connectors
extern DigitalIn ConnectorDI6;              ///< DI-6 connector instance
extern DigitalIn ConnectorDI7;              ///< DI-7 connector instance
extern DigitalIn ConnectorDI8;              ///< DI-8 connector instance

// Analog/Digital Inputs
extern DigitalInAnalogIn ConnectorA9;       ///< A-9 connector instance
extern DigitalInAnalogIn ConnectorA10;      ///< A-10 connector instance
extern DigitalInAnalogIn ConnectorA11;      ///< A-11 connector instance
extern DigitalInAnalogIn ConnectorA12;      ///< A-12 connector instance

// Motor Connectors
extern MotorDriver ConnectorM0;             ///< M-0 connector instance
extern MotorDriver ConnectorM1;             ///< M-1 connector instance
extern MotorDriver ConnectorM2;             ///< M-2 connector instance
extern MotorDriver ConnectorM3;             ///< M-3 connector instance

// Serial Port connectors
extern SerialUsb    ConnectorUsb;           ///< USB connector instance
extern SerialDriver ConnectorCOM0;          ///< COM-0 connector instance
extern SerialDriver ConnectorCOM1;          ///< COM-1 connector instance

/// Ethernet manager
extern EthernetManager &EthernetMgr;

/// CCIO-8 manager
extern CcioBoardManager &CcioMgr;

/// Motor connector manager
extern MotorManager &MotorMgr;

/// ADC module manager
extern AdcManager &AdcMgr;

/// Input manager
extern InputManager &InputMgr;

/// Xbee wireless
extern XBeeDriver XBee;

/// Position Decoder
extern EncoderInput EncoderIn;

/// Status manager
extern StatusManager &StatusMgr;

/// Timing manager
extern SysTiming &TimingMgr;

/// SD card
extern SdCardDriver SdCard;

/// System manager
extern SysManager SysMgr;
}

using namespace ClearCore;

#endif // __CLEARCORE_H__
