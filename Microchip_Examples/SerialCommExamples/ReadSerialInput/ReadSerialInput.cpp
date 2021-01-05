/*
 * Title: ReadSerialInput
 *
 * Objective:
 *    This example demonstrates how to read and display incoming data from a
 *    serial port.
 *
 * Description:
 *    This example will read one byte per second from the serial input buffer.
 *    During operation, if a byte has been received, it will be printed to the
 *    USB serial port as a character.
 *
 * Requirements:
 * ** A serial input source connected to COM-0.
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
#define baudRateSerialPort 115200
#define baudRateInputPort  115200

// When using COM ports, is the device TTL or RS232?
#define isTtlInputPort  false

// Specify which serial interface to use as output.
#define SerialPort ConnectorUsb

// Specify which serial interface to use as input.
#define InputPort ConnectorCOM0

// Container for the byte to be read-in
int16_t input;

int main() {
    // Set up serial communication to print out the serial input.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRateSerialPort);
    SerialPort.PortOpen();
    while (!SerialPort) {
        continue;
    }

    // Set up serial communication to send serial input over.
    if (isTtlInputPort) {
        InputPort.Mode(Connector::TTL);
    }
    else {
        InputPort.Mode(Connector::RS232);
    }
    InputPort.Speed(baudRateInputPort);
    InputPort.PortOpen();
    while (!InputPort) {
        continue;
    }

    while (true) {
        // Read the input.
        input = InputPort.CharGet();

        // If there was a valid byte read-in, print it.
        if (input != -1) {
            // Display the input character received.
            SerialPort.Send("Received: ");
            SerialPort.SendLine((char)input);
        }
        else {
            SerialPort.SendLine("No data received...");
        }

        // Wait a second then repeat...
        Delay_ms(1000);
    }
}
