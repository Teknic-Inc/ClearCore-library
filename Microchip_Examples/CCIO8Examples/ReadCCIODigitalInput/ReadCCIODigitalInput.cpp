/*
 * Title: ReadCCIODigitalInput
 *
 * Objective:
 *    This example demonstrates how to initialize a CCIO-8 Expansion Board and
 *    read from one of its inputs.
 *
 * Description:
 *    This example sets up COM-0 to control a CCIO-8 Expansion Board then reads
 *    the state of an input on the CCIO-8's connector 0. During operation, the
 *    state of the input is printed to the USB serial port.
 *
 * Requirements:
 * ** A CCIO-8 Expansion Board powered and connected to COM-0.
 * ** An digital input device such as a switch connected to the CCIO-8's
 *    connector 0.
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

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Specify which ClearCore serial COM port is connected to the COM IN port
// of the CCIO-8 board: ConnectorCOM0 or ConnectorCOM1.
#define CcioPort ConnectorCOM0

// Select the baud rate to match the target serial device.
#define baudRate 9600

int main() {
    // Set up serial communication to display CCIO-8 state.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    SerialPort.PortOpen();
    while (!SerialPort && Milliseconds() - startTime < timeout) {
        continue;
    }

    // Set up the CCIO-8 COM port.
    CcioPort.Mode(Connector::CCIO);
    CcioPort.PortOpen();

    // Make sure the input connector is in input mode (the default for all
    // CCIO-8 pins).
    CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->Mode(Connector::INPUT_DIGITAL);

    while (true) {
        // Read the state of the input connector
        int16_t state = CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)->State();

        // Display the state of the input connector.
        SerialPort.Send("CCIOA0 Input state: ");
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
