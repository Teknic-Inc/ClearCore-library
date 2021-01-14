/*
 * Title: WriteDigitalOutput
 *
 * Objective:
 *    This example demonstrates how to write the state of a ClearCore digital
 *    output.
 *
 * Description:
 *    This example repeatedly toggles the state of the ClearCore's six digital
 *    outputs, IO-0 through IO-5.
 *
 * Requirements:
 * ** A device that takes in a digital signal connected to any of the I/O's,
 *    IO-0 through IO-5.
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

// Declares a variable used to write new states to the output. We will toggle
// this true/false.
bool outputState;

int main() {
    // Configure pins IO-0 through IO-5 as digital outputs. These are the only
    // pins that support digital output mode.
    ConnectorIO0.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO1.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO2.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO3.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO4.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO5.Mode(Connector::OUTPUT_DIGITAL);

    // The connectors are all set up; start the loop with turning them all on.
    outputState = true;

    while (true) {
        // Toggle the digital output state.
        if (outputState) {
            ConnectorIO0.State(true);
            ConnectorIO1.State(true);
            ConnectorIO2.State(true);
            ConnectorIO3.State(true);
            ConnectorIO4.State(true);
            ConnectorIO5.State(true);
        }
        else {
            ConnectorIO0.State(false);
            ConnectorIO1.State(false);
            ConnectorIO2.State(false);
            ConnectorIO3.State(false);
            ConnectorIO4.State(false);
            ConnectorIO5.State(false);
        }

        // Toggle the state to write in the next loop.
        outputState = !outputState;

        // Wait a second, then repeat.
        Delay_ms(1000);
    }
}
