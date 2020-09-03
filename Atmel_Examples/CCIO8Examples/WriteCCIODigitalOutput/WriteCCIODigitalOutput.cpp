/*
 * Title: WriteCCIODigitalOutput
 *
 * Objective:
 *    This example demonstrates how to initialize a CCIO-8 Expansion Board and
 *    write to its outputs.
 *
 * Description:
 *    This example sets up COM-0 to control a CCIO-8 Expansion Board then
 *    toggles the state of all of the CCIO-8's outputs from true to false.
 *
 * Requirements:
 * ** A CCIO-8 Expansion Board powered and connected to COM-0.
 * ** An output such as an LED connected to one or more of the CCIO-8's
 *    connectors.
 *      Note: You can leave the I/O points disconnected and still see the
 *      built-in I/O LEDs toggle with the connector state.
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

// Specify which ClearCore serial COM port is connected to the COM IN port
// of the CCIO-8 board: ConnectorCOM0 or ConnectorCOM1.
#define CcioPort ConnectorCOM0

// The state to be written to each output connector.
bool outputState;

int main() {
    // Set up the CCIO-8 COM port.
    CcioPort.Mode(Connector::CCIO);
    CcioPort.PortOpen();

    // Using the CCIO-8 Board Manager, configure each connector on a single
    // ClearCore I/O Expansion Board as an output. They can be either digital
    // inputs or digital outputs.
    // Note: If there is more than one CCIO-8 in the link, you may want to
    // to change the limit of the for-loop from "CLEARCORE_PIN_CCIOA7" to the
    // value of the last CCIO-8 connector to accommodate the additional boards.
    // For example, if you are using three CCIO-8 boards, change this value to
    // "CLEARCORE_PIN_CCIOC7", and do the same for the for-loop in the loop()
    // function below.
    for (int16_t pin = CLEARCORE_PIN_CCIOA0; pin <= CLEARCORE_PIN_CCIOA7; pin++) {
        CcioMgr.PinByIndex(static_cast<ClearCorePins>(pin))->Mode(
            Connector::OUTPUT_DIGITAL);
    }

    // The connectors are all set up; start the loop with turning them all on.
    outputState = true;

    while (true) {
        // Send the current state to each of the outputs.
        for (int16_t pin = CLEARCORE_PIN_CCIOA0; pin <= CLEARCORE_PIN_CCIOA7; pin++) {
            CcioMgr.PinByIndex(static_cast<ClearCorePins>(pin))->State(outputState);
        }

        // Toggle the state to be written next time.
        outputState = !outputState;

        // Wait 1 second.
        Delay_ms(1000);
    }
}
