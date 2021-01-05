/*
 * Title: ReadDigitalInput
 *
 * Objective:
 *    This example demonstrates how to read the state of a ClearCore digital
 *    input.
 *
 * Description:
 *    This example repeatedly reads the state of a defined digital input. During
 *    operation, the state of the input is printed to the USB serial port.
 *
 * Requirements:
 * ** A digital input device, such as a switch or sensor, connected to DI-6
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

// Specify which input pin to read from.
// IO-0 through A-12 are all available as digital inputs.
#define inputPin ConnectorDI6

// The current state of the input pin
int16_t state;

// Select the baud rate to match the target serial device.
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
        // Read the state of the input connector.
        state = inputPin.State();

        // Display the state of the input connector.
        SerialPort.Send("DI-6 Input state: ");
        if (state) {
            SerialPort.SendLine("ON");
        }
        else {
            SerialPort.SendLine("OFF");
        }

        // Wait a second then repeat...
        Delay_ms(1000);
    }
}
