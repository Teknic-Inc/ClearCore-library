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

#include "StatusManager.h"
#include <stddef.h>
#include <stdio.h>
#include "atomic_utils.h"
#include "AdcManager.h"
#include "CcioBoardManager.h"
#include "DigitalInOutHBridge.h"
#include "EthernetManager.h"
#include "HardwareMapping.h"
#include "MotorDriver.h"
#include "NvmManager.h"
#include "SdCardDriver.h"
#include "SysConnectors.h"
#include "SysTiming.h"

namespace ClearCore {

extern volatile uint32_t tickCnt;
extern DigitalInOutHBridge *const hBridgeCon[];
extern MotorDriver *const MotorConnectors[];
extern AdcManager &AdcMgr;
extern CcioBoardManager &CcioMgr;
extern EthernetManager &EthernetMgr;
extern NvmManager &NvmMgr;
extern ShiftRegister ShiftReg;
extern SdCardDriver SdCard;
StatusManager &StatusMgr = StatusManager::Instance();

#define OFFBOARD_5V_TRIP_V 4.0
#define OVER_VOLTAGE_TRIP_V 32.0
#define UNDER_VOLTAGE_TRIP_V 10.0
#define OFFBOARD_5V_EXIT_V 4.5
#define OVER_VOLTAGE_EXIT_V 28.0
#define UNDER_VOLTAGE_EXIT_V 11.0

// calc: TripInVolts * (1 << 15) / MaxReadoutInVolts
#define OFFBOARD_5V_TRIP_CNT ((uint16_t)(OFFBOARD_5V_TRIP_V * (1 << 15) / \
   AdcManager::ADC_CHANNEL_MAX_FLOAT[AdcManager::ADC_5VOB_MON]))
#define OVER_VOLTAGE_TRIP_CNT ((uint16_t)(OVER_VOLTAGE_TRIP_V * (1 << 15) / \
   AdcManager::ADC_CHANNEL_MAX_FLOAT[AdcManager::ADC_VSUPPLY_MON]))
#define UNDER_VOLTAGE_TRIP_CNT ((uint16_t)(UNDER_VOLTAGE_TRIP_V * (1 << 15) / \
   AdcManager::ADC_CHANNEL_MAX_FLOAT[AdcManager::ADC_VSUPPLY_MON]))
#define OFFBOARD_5V_EXIT_CNT ((uint16_t)(OFFBOARD_5V_EXIT_V * (1 << 15) / \
   AdcManager::ADC_CHANNEL_MAX_FLOAT[AdcManager::ADC_5VOB_MON]))
#define OVER_VOLTAGE_EXIT_CNT ((uint16_t)(OVER_VOLTAGE_EXIT_V * (1 << 15) / \
   AdcManager::ADC_CHANNEL_MAX_FLOAT[AdcManager::ADC_VSUPPLY_MON]))
#define UNDER_VOLTAGE_EXIT_CNT ((uint16_t)(UNDER_VOLTAGE_EXIT_V * (1 << 15) / \
   AdcManager::ADC_CHANNEL_MAX_FLOAT[AdcManager::ADC_VSUPPLY_MON]))


// Ensures that only one instance of StatusManager is ever created.
StatusManager &StatusManager::Instance() {
    static StatusManager *instance = new StatusManager;
    return *instance;
}

StatusManager::StatusRegister StatusManager::StatusRT(StatusRegister mask) {
    StatusRegister statusReg;
    statusReg.reg = atomic_load_n(&m_statusRegRT.reg) & mask.reg;
    return statusReg;
}

StatusManager::StatusRegister StatusManager::StatusRisen(StatusRegister mask) {
    StatusRegister statusReg;
    statusReg.reg =
        atomic_fetch_and(&m_statusRegRisen.reg, ~mask.reg) & mask.reg;
    return statusReg;
}

StatusManager::StatusRegister StatusManager::StatusFallen(StatusRegister mask) {
    StatusRegister statusReg;
    statusReg.reg =
        atomic_fetch_and(&m_statusRegFallen.reg, ~mask.reg) & mask.reg;
    return statusReg;
}

StatusManager::StatusRegister StatusManager::StatusAccum(StatusRegister mask) {
    StatusRegister statusReg;
    statusReg.reg =
        atomic_fetch_and(&m_statusRegAccum.reg, ~mask.reg) & mask.reg;
    atomic_exchange_n(&m_statusRegAccum.reg, atomic_load_n(&m_statusRegRT.reg));
    return statusReg;
}

StatusManager::StatusRegister StatusManager::SinceStartupAccum(
    StatusRegister mask) {
    StatusRegister statusReg;
    statusReg.reg = atomic_load_n(&m_statusRegSinceStartup.reg) & mask.reg;
    return statusReg;
}

bool StatusManager::AdcIsInTimeout() {
    StatusRegister statusReg = StatusRT();
    return statusReg.bit.AdcTimeout;
}

SysConnectorState StatusManager::IoOverloadRT(SysConnectorState mask) {
    SysConnectorState overloadReg;
    overloadReg.reg = atomic_load_n(&m_overloadRT.reg) & mask.reg;
    return overloadReg;
}

SysConnectorState StatusManager::IoOverloadAccum(SysConnectorState mask) {
    SysConnectorState overloadReg;
    overloadReg.reg =
        atomic_fetch_and(&m_overloadAccum.reg, ~mask.reg) & mask.reg;
    m_overloadAccum.reg = m_overloadRT.reg;
    return overloadReg;
}

SysConnectorState StatusManager::IoOverloadSinceStartupAccum(
    SysConnectorState mask) {
    SysConnectorState overloadReg;
    overloadReg.reg = atomic_load_n(&m_overloadSinceStartup.reg) & mask.reg;
    return overloadReg;
}

inline bool Offboard5VCheck(bool currentStatus) {
    return (currentStatus &&
            AdcMgr.FilteredResult(AdcManager::ADC_5VOB_MON) <
            OFFBOARD_5V_EXIT_CNT) || (!currentStatus &&
                                      AdcMgr.FilteredResult(AdcManager::ADC_5VOB_MON) <
                                      OFFBOARD_5V_TRIP_CNT);
}

inline bool VSupplyOverVoltageCheck(bool currentStatus) {
    return (currentStatus &&
            AdcMgr.FilteredResult(AdcManager::ADC_VSUPPLY_MON) >=
            OVER_VOLTAGE_EXIT_CNT) || (!currentStatus &&
                                       AdcMgr.FilteredResult(AdcManager::ADC_VSUPPLY_MON) >=
                                       OVER_VOLTAGE_TRIP_CNT);
}

inline bool VSupplyUnderVoltageCheck(bool currentStatus) {
    return (currentStatus &&
            AdcMgr.FilteredResult(AdcManager::ADC_VSUPPLY_MON) <
            UNDER_VOLTAGE_EXIT_CNT) || (!currentStatus &&
                                        AdcMgr.FilteredResult(AdcManager::ADC_VSUPPLY_MON) <
                                        UNDER_VOLTAGE_TRIP_CNT);
}

inline bool HBridgeFaultCheck() {
    return (!static_cast<bool>(PORT->Group[OutFault_04or05.gpioPort].IN.reg &
                               (1UL << OutFault_04or05.gpioPin)));
}

bool StatusManager::Initialize(ShiftRegister::Masks faultLed) {
    m_faultLed = faultLed;
    m_disableMotors = false;
    m_statusRegSinceStartup = 0;
    ShiftReg.DiagnosticLedSweep();

    return true;
}

void StatusManager::Refresh() {
    // Temporary holder for status fields while being populated.
    // Write all values to temp then do one atomic exchange
    StatusRegister statusPrev;
    StatusRegister statusPending;

    // Load contents of the real-time status register into statusPrev
    atomic_load(&m_statusRegRT.reg, &statusPrev.reg);

    // Load the contents of statusPending with current status information
    statusPending.bit.VSupplyOverVoltage =
        VSupplyOverVoltageCheck(statusPrev.bit.VSupplyOverVoltage);
    statusPending.bit.VSupplyUnderVoltage =
        VSupplyUnderVoltageCheck(statusPrev.bit.VSupplyUnderVoltage);
    statusPending.bit.Overloaded5V =
        Offboard5VCheck(statusPrev.bit.Overloaded5V);
    statusPending.bit.HBridgeOverloaded = HBridgeFaultCheck();
    statusPending.bit.HBridgeReset = m_hbridgeResetting;
    statusPending.bit.AdcTimeout = AdcMgr.AdcTimeout();
    statusPending.bit.OutputOverloaded =
        static_cast<bool>(ShiftReg.OverloadActive());
    statusPending.bit.CcioLinkBroken = CcioMgr.LinkBroken();
    statusPending.bit.CcioOverloaded = CcioMgr.IoOverloadRT();
    statusPending.bit.EthernetDisconnect = !EthernetMgr.PhyLinkActive();
    statusPending.bit.EthernetRemoteFault = EthernetMgr.PhyRemoteFault();
    statusPending.bit.EthernetPhyInitFailed = EthernetMgr.PhyInitFailed();
    statusPending.bit.SdCardError = SdCard.IsInFault();
    statusPending.bit.NvmDesync = !NvmMgr.Synchonized();

    UpdateBlinkCodes(statusPending);

    if (hBridgeCon[0]->Mode() != Connector::OUTPUT_DIGITAL) {
        ShiftReg.LedInFault(ShiftRegister::SR_LED_IO_4_MASK,
                            statusPending.bit.HBridgeOverloaded);
    }
    if (hBridgeCon[1]->Mode() != Connector::OUTPUT_DIGITAL) {
        ShiftReg.LedInFault(ShiftRegister::SR_LED_IO_5_MASK,
                            statusPending.bit.HBridgeOverloaded);
    }

    // Write the updates in statusPending to the real-time register
    atomic_load(&statusPending.reg, &m_statusRegRT.reg);

    // Update the edge detection registers
    atomic_or_fetch(&m_statusRegFallen.reg,
                    statusPrev.reg & ~statusPending.reg);
    atomic_or_fetch(&m_statusRegRisen.reg,
                    ~statusPrev.reg & statusPending.reg);

    // Update the accumulating registers
    atomic_or_fetch(&m_statusRegAccum.reg, statusPending.reg);
    atomic_or_fetch(&m_statusRegSinceStartup.reg, statusPending.reg);

    bool disableMotorsPrev = m_disableMotors;

    // Disable the MotorDrivers when the Vbus is overloaded, or the HBridge is
    // resetting. When HBridge is resetting, the reading for Vbus is cut off
    // and cannot be read. To prevent an undetected overvoltage, disable motors.
    m_disableMotors = m_statusRegRT.bit.VSupplyOverVoltage |
                      m_statusRegRT.bit.HBridgeReset;

    // Set the motor disableMotors transition
    if (disableMotorsPrev != m_disableMotors) {
        for (uint8_t i = 0; i < MOTOR_CON_CNT; i++) {
            MotorConnectors[i]->FaultState(m_disableMotors);
        }
        for (uint8_t i = 0; i < HBRIDGE_CON_CNT; i++) {
            hBridgeCon[i]->FaultState(m_disableMotors);
        }
    }
}

void StatusManager::HBridgeReset() {
    HBridgeState(true);

    // Wait 2 System Ticks for the Refresh function to come through
    // and force all motor outputs to disable
    uint32_t tickStart = tickCnt;
    while (tickCnt - tickStart < 2) {
        continue;
    }

    HBridgeState(false);
}

void StatusManager::HBridgeState(bool reset) {
    ClearCorePorts port = Vsupply_MON_IO_4and5_RST.gpioPort;
    uint32_t pin = Vsupply_MON_IO_4and5_RST.gpioPin;
    StatusRegister resetBit;
    resetBit.bit.HBridgeReset = 1;
    m_hbridgeResetting = reset;

    if (reset) {
        atomic_or_fetch(&m_statusRegRT.reg, resetBit.reg);
        atomic_or_fetch(&m_statusRegRisen.reg, resetBit.reg);
        // Configure the pin for I/O
        PIN_CONFIGURATION(port, pin, PORT_PINCFG_INEN);
        // Set the pin to output HIGH
        DATA_OUTPUT_STATE(port, 1UL << pin, true);
        // Set the pin as an output
        DATA_DIRECTION_OUTPUT(port, 1UL << pin);
    }
    else {
        // Set the pin as an input
        DATA_DIRECTION_INPUT(port, 1UL << pin);
        // Enable the Peripheral Multiplexer
        PMUX_ENABLE(port, pin);
    }
}

void StatusManager::BlinkCode(BlinkCodeDriver::BlinkCodeGroups group,
                              uint8_t mask) {
    m_blinkMgr.CodeGroupAdd(group, mask);
}

void StatusManager::UpdateBlinkCodes(StatusRegister status) {
    if (status.bit.VSupplyOverVoltage) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_SUPPLY_ERROR,
            BlinkCodeDriver::SUPPLY_ERROR_VSUPPLY_HIGH);
    }
    if (status.bit.VSupplyUnderVoltage) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_SUPPLY_ERROR,
            BlinkCodeDriver::SUPPLY_ERROR_VSUPPLY_LOW);
    }
    if (status.bit.HBridgeOverloaded) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_DEVICE_ERROR,
            BlinkCodeDriver::DEVICE_ERROR_HBRIDGE);
    }
    if (status.bit.Overloaded5V) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_SUPPLY_ERROR,
            BlinkCodeDriver::SUPPLY_ERROR_5VOB_OVERLOAD);
    }
    if (status.bit.SdCardError) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_DEVICE_ERROR,
            BlinkCodeDriver::DEVICE_ERROR_SD_CARD);
    }
    // Only report Ethernet problems if we called EthernetManager::Setup()
    // and expect Ethernet to be functional.
    if (EthernetMgr.EthernetActive() &&
            (status.bit.EthernetRemoteFault ||
             status.bit.EthernetPhyInitFailed)) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_DEVICE_ERROR,
            BlinkCodeDriver::DEVICE_ERROR_ETHERNET);
    }
    if (status.bit.CcioLinkBroken) {
        m_blinkMgr.CodeGroupAdd(
            BlinkCodeDriver::BLINK_GROUP_DEVICE_ERROR,
            BlinkCodeDriver::DEVICE_ERROR_CCIO);
    }

    m_blinkMgr.Update();
    ShiftReg.BlinkCode(m_blinkMgr.CodePresent(), m_blinkMgr.LedState());
}

void StatusManager::OverloadUpdate(uint32_t mask, bool inFault) {
    if (inFault) {
        m_overloadRT.reg |= mask;
    }
    else {
        m_overloadRT.reg &= ~mask;
    }
    m_overloadSinceStartup.reg |= m_overloadRT.reg;
    m_overloadAccum.reg |= m_overloadRT.reg;
}

}