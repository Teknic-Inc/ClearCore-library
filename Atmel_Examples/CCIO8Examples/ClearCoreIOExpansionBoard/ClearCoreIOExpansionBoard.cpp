/*
 * Title: ClearCoreIOExpansionBoard
 *
 * Objective:
 *    This example demonstrates how to initialize a CCIO-8 Expansion Board and
 *    read from or write to it. Up to 8 total CCIO-8 boards can be used.
 *
 * Description:
 *    This example sets up COM-0 to control up to 8 CCIO-8 Expansion Boards,
 *    sets all CCIO-8 connectors to be either inputs or outputs depending on the
 *    selected mode. In input mode, pin statuses are printed to the USB serial
 *    port. In output mode, all pins outputs are sequentially turned on then off.
 *
 * Requirements:
 * ** A CCIO-8 Expansion Board, with power wired, and connected to COM-0. Any
 *    other CCIO-8 boards should be chained off of this first board
 * ** Edit the value of "inputMode" below to select input mode or output mode
 * ** (For input mode) A number of inputs, like switches, wired to CCIO-8
 *    connectors.
 * ** (For output mode) A number of outputs, like LEDs, wired to CCIO-8
 *    connectors.
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

// Specify which ClearCore serial COM port is connected to the "COM IN" port
// of the CCIO-8 board. COM-1 may also be used.
#define CcioPort ConnectorCOM0

// Specify which serial to use: ConnectorUsb, ConnectorCOM0, or ConnectorCOM1.
#define SerialPort ConnectorUsb

// Set this flag to true to use the CCIO-8 connectors as digital inputs.
// Set it to false to use the CCIO-8 connectors as digital outputs.
const bool inputMode = true;

uint8_t ccioBoardCount;     // Store the number of connected CCIO-8 boards here.
uint8_t ccioPinCount;       // Store the number of connected CCIO-8 pins here.

// Select the baud rate to match the target device.
#define baudRate 9600

// These will be used to format the text that is printed to the serial port.
#define MAX_MSG_LEN 80
char msg[MAX_MSG_LEN + 1];

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

    // Initialize the CCIO-8 board.
    ccioBoardCount = CcioMgr.CcioCount();
    // CCIO_PINS_PER_BOARD is a constant defined in the main ClearCore library
    // that evaluates to 8.
    ccioPinCount = ccioBoardCount * CCIO_PINS_PER_BOARD;

    // Print the number of discovered CCIO-8 boards to the serial port.
    snprintf(msg, MAX_MSG_LEN, "Discovered %d CCIO-8 board", ccioBoardCount);
    SerialPort.Send(msg);

    if (ccioBoardCount != 1) {
        SerialPort.Send("s");
    }
    SerialPort.SendLine("...");
    SerialPort.SendLine();

    if (!inputMode) {
        // Set each CCIO-8 pin to the correct mode. The CCIO-8 pins default to
        // being an input so the pin mode doesn't need to be set for input mode.
        for (uint8_t ccioPinIndex = 0; ccioPinIndex < ccioPinCount; ccioPinIndex++) {
            CcioMgr.PinByIndex(static_cast<ClearCorePins>(CLEARCORE_PIN_CCIOA0 +
                               ccioPinIndex))->Mode(Connector::OUTPUT_DIGITAL);
        }
    }

    while (true) {
        // Make sure the CCIO-8 link is established.
        if (CcioMgr.LinkBroken()) {
            uint32_t lastStatusTime = Milliseconds();
            SerialPort.SendLine("The CCIO-8 link is broken!");
            // Make sure the CCIO-8 link is established.
            while (CcioMgr.LinkBroken()) {
                if (Milliseconds() - lastStatusTime > 1000) {
                    SerialPort.SendLine("The CCIO-8 link is still broken!");
                    lastStatusTime = Milliseconds();
                }
            }
            SerialPort.SendLine("The CCIO-8 link is online again!");
        }

        // ClearCore can automatically detect when the number of attached CCIO-8
        // boards changes. Check to see if there's been a change.
        uint8_t newBoardCount = CcioMgr.CcioCount();
        if (ccioBoardCount != newBoardCount) {
            snprintf(msg, MAX_MSG_LEN, "CCIO-8 board count changed from %d to %d.",
                     ccioBoardCount, newBoardCount);
            SerialPort.SendLine(msg);
            ccioPinCount = newBoardCount * CCIO_PINS_PER_BOARD;
            ccioBoardCount = newBoardCount;
        }

        // With one CCIO-8 board attached, we have control over eight additional
        // digital I/O connectors. On these connectors, we can perform digital
        // reads and non-PWM digital writes.
        if (inputMode) {
            // Read the digital state of CCIO-8 connectors 0 through 7 as inputs.
            for (uint8_t ccioPinIndex = 0; ccioPinIndex < CCIO_PINS_PER_BOARD;
                    ccioPinIndex++) {
                uint16_t state = CcioMgr.PinByIndex(static_cast<ClearCorePins>
                                     (CLEARCORE_PIN_CCIOA0 + ccioPinIndex))->State();

                snprintf(msg, MAX_MSG_LEN, "CCIO-A%d:   ", ccioPinIndex);
                SerialPort.Send(msg);

                if (state) {
                    SerialPort.SendLine("ON");
                }
                else {
                    SerialPort.SendLine("OFF");
                }
            }

            // If you have multiple CCIO-8 boards attached, individual printouts
            // become hard to read. We can access all of the CCIO-8 bits at once
            // and print them out in hex.
            snprintf(msg, MAX_MSG_LEN, "All CCIO-8 Inputs:   0x%0*llX",
                     ccioBoardCount * 2, CcioMgr.InputState());
            SerialPort.SendLine(msg);

            // The application may be only interested in the transitions of an
            // input. We can read the rise and fall registers and print them out.
            snprintf(msg, MAX_MSG_LEN, "CCIO-8 Input Rise:   0x%0*llX",
                     ccioBoardCount * 2, CcioMgr.InputsRisen());
            SerialPort.SendLine(msg);
            snprintf(msg, MAX_MSG_LEN, "CCIO-8 Input Fall:   0x%0*llX",
                     ccioBoardCount * 2, CcioMgr.InputsFallen());
            SerialPort.SendLine(msg);

            SerialPort.SendLine("---------------------");

            Delay_ms(1000);
        }
        else {
            // Write digital HIGH then digital LOW to each CCIO-8 connector.

            SerialPort.SendLine("Writing digital HIGH to each CCIO-8 connector...");

            for (uint8_t ccioPinIndex = 0; ccioPinIndex < ccioPinCount; ccioPinIndex++) {
                CcioMgr.PinByIndex(static_cast<ClearCorePins>(
                    CLEARCORE_PIN_CCIOA0 + ccioPinIndex))->State(true);
                Delay_ms(500);
            }

            SerialPort.SendLine("Writing digital LOW to each CCIO-8 connector...");

            for (int8_t ccioPinIndex = ccioPinCount - 1; ccioPinIndex >= 0; ccioPinIndex--) {
                CcioMgr.PinByIndex(static_cast<ClearCorePins>(
                    CLEARCORE_PIN_CCIOA0 + ccioPinIndex))->State(false);
                Delay_ms(500);
            }
        }
    }
}
