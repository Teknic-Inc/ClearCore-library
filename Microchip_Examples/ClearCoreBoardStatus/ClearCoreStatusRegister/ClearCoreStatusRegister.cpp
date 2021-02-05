/*
 * Title: ClearCoreStatusRegister
 *
 * Objective:
 *    This example demonstrates how to read and display bits in the ClearCore
 *    Status Register.
 *
 * Description:
 *    This example gets a snapshot of the ClearCore's real-time status register
 *    and prints the state of the status register bits to the USB serial port.
 *
 * Requirements:
 * ** None
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// Select the baud rate to match the target serial device
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

int main() {
    // Set up serial communication at a baud rate of 9600 bps then wait up to
    // 5 seconds for a port to open.
    // Serial communication is not required for this example to run, however the
    // example will appear to do nothing without serial output.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    SerialPort.PortOpen();
    while (!SerialPort && Milliseconds() - startTime < timeout) {
        continue;
    }

    while (true) {
        // Get a copy of the real-time status register.
        StatusManager::StatusRegister statusReg = StatusMgr.StatusRT();

        SerialPort.SendLine("Status Register:");

        SerialPort.Send("Vsupply over-voltage:\t\t");
        if (statusReg.bit.VSupplyOverVoltage) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("Vsupply under-voltage:\t\t");
        if (statusReg.bit.VSupplyUnderVoltage) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("H-Bridge output overloaded:\t");
        if (statusReg.bit.HBridgeOverloaded) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("H-Bridge resetting:\t\t");
        if (statusReg.bit.HBridgeReset) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        // This status bit denotes the state of the 5 volt supply for off-board
        // items
        SerialPort.Send("Offboard 5V overloaded:\t\t");
        if (statusReg.bit.Overloaded5V) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("Output overloaded:\t\t");
        if (statusReg.bit.OutputOverloaded) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("CCIO-8 output overloaded:\t");
        if (statusReg.bit.CcioOverloaded) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("CCIO-8 link broken:\t\t");
        if (statusReg.bit.CcioLinkBroken) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("ADC in timeout:\t\t\t");
        if (statusReg.bit.AdcTimeout) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("Ethernet disconnect:\t\t");
        if (statusReg.bit.EthernetDisconnect) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("Ethernet remote fault:\t\t");
        if (statusReg.bit.EthernetRemoteFault) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.Send("SD card error:\t\t\t");
        if (statusReg.bit.SdCardError) {
            SerialPort.SendLine('1');
        }
        else {
            SerialPort.SendLine('0');
        }

        SerialPort.SendLine("------------------------");

        // Wait a couple seconds then repeat...
        Delay_ms(2000);
    }
}
