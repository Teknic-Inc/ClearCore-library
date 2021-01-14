/*
 * Title: MotorStatusRegister
 *
 * Objective:
 *    This example demonstrates how to read and display bits in the ClearCore's
 *    MotorDriver status register.
 *
 * Description:
 *    This example gets a snapshot of the status register for each MotorDriver
 *    connector with an attached motor. Then, the state of the status register
 *    bits is printed to the USB serial port.
 *
 * Requirements:
 * ** A ClearPath motor must be connected to Connector M-0.
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

// Select the baud rate to match the target device.
#define baudRate 9600

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// The containers for our motor objects. If only some of the motor connectors
// are being used, remove the unused entries from the following arrays and
// reduce motorConnectorCount.
MotorDriver *motorConnectors[] = {&ConnectorM0, &ConnectorM1,
                                  &ConnectorM2, &ConnectorM3
                                 };
char motorConnectorNames[][4] = { "M-0", "M-1", "M-2", "M-3" };
uint8_t motorConnectorCount = 4;

// Hold a string representation of each motor's ready state.
char *readyStateStr;

// Declare our helper function so that it may be used before it is defined.
char *ReadyStateString(MotorDriver::MotorReadyStates readyState);

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
        // Get a copy of the motor status register for each motor connector.
        for (uint8_t i = 0; i < motorConnectorCount; i++) {
            MotorDriver *motor = motorConnectors[i];
            volatile const MotorDriver::StatusRegMotor &statusReg = motor->StatusReg();

            SerialPort.Send("Motor Status Register for ");
            SerialPort.Send(motorConnectorNames[i]);
            SerialPort.SendLine(":");

            SerialPort.Send("Enabled:\t\t");
            if (statusReg.bit.Enabled) {
                SerialPort.SendLine('1');
            }
            else {
                SerialPort.SendLine('0');
            }

            SerialPort.Send("Move direction:\t\t");
            if (statusReg.bit.MoveDirection) {
                SerialPort.SendLine('+');
            }
            else {
                SerialPort.SendLine('-');
            }

            SerialPort.Send("Steps active:\t\t");
            if (statusReg.bit.StepsActive) {
                SerialPort.SendLine('1');
            }
            else {
                SerialPort.SendLine('0');
            }

            SerialPort.Send("At velocity target:\t");
            if (statusReg.bit.AtTargetVelocity) {
                SerialPort.SendLine('1');
            }
            else {
                SerialPort.SendLine('0');
            }

            SerialPort.Send("Ready state:\t\t");
            readyStateStr = ReadyStateString(statusReg.bit.ReadyState);
            SerialPort.SendLine(readyStateStr);

            SerialPort.SendLine("--------------------------------");
        }

        // Wait a few seconds then repeat...
        Delay_ms(5000);
    }
}

/*------------------------------------------------------------------------------
 * ReadyStateString
 *
 *    Converts the state of a motor status register bit into a user-readable
 *    format so it may be printed to a serial port.
 *
 * Parameters:
 *    MotorReadyStates readyState  - The current state of the ReadyState bit
 *
 * Returns: Text describing the state of the status bit.
 */
char *ReadyStateString(MotorDriver::MotorReadyStates readyState) {
    switch (readyState) {
        case MotorDriver::MOTOR_DISABLED:
            return (char *)"Disabled";
        case MotorDriver::MOTOR_ENABLING:
            return (char *)"Enabling";
        case MotorDriver::MOTOR_FAULTED:
            return (char *)"Faulted";
        case MotorDriver::MOTOR_READY:
            return (char *)"Ready";
        case MotorDriver::MOTOR_MOVING:
            return (char *)"Moving";
        default:
            // Something has gone wrong if this is printed
            return (char *)"???";
    }
}
//------------------------------------------------------------------------------
