/*
 * Title: WriteHBridgeOutput
 *
 * Objective:
 *    This example demonstrates how to output a differential/bi-directional PWM
 *    signal from a ClearCore H-Bridge connector.
 *
 * Description:
 *    This example sets up a ClearCore H-Bridge connector for H-Bridge output,
 *    then repeatedly ramps the PWM duty cycle output up and down, both sourcing
 *    and sinking current.
 *
 * Requirements:
 * ** A device capable of receiving an H-Bridge bi-directional PWM signal, like
 *    a bi-directional brushed DC motor, connected to IO-4. Refer to the
 *    ClearCore System Diagram on how to wire a device to the H-Bridge capable
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

// Defines which H-Bridge capable connector to use: ConnectorIO4 or ConnectorIO5
#define HBridgeConnector ConnectorIO4

// Declares our user-defined helper function, which is used to check for
// overloads and reset the H-Bridge output as necessary. The definition/
// implementation of this function is at the bottom of the example.
void CheckHBridgeOverload();

int main() {
    // H-Bridge output is supported on connectors IO-4 and IO-5 only.
    // Set the H-Bridge connector into H-Bridge output mode.
    HBridgeConnector.Mode(Connector::OUTPUT_H_BRIDGE);

    while (true) {
        // Output bi-directional PWM on the H-Bridge connector over the full range
        // of output values/duty cycles (-INT16_MAX-1 to INT16_MAX).

        // Positive values (between 1 and INT16_MAX) will sink current into the
        // signal pin.
        // Negative values (between -1 and -INT16_MAX-1) will source current
        // from the signal pin.

        for (int16_t i = 0; i < INT16_MAX; i++) {
            // Check for overloads and reset if needed
            CheckHBridgeOverload();

            // Write to the output.
            HBridgeConnector.State(i);
            Delay_us(125);
        }

        for (int16_t i = 0; i < INT16_MAX; i++) {
            CheckHBridgeOverload();

            HBridgeConnector.State(INT16_MAX - i);
            Delay_us(125);
        }

        for (int16_t i = 0; i < INT16_MAX; i++) {
            CheckHBridgeOverload();

            HBridgeConnector.State(-i);
            Delay_us(125);
        }

        for (int16_t i = 0; i < INT16_MAX; i++) {
            CheckHBridgeOverload();

            HBridgeConnector.State(-INT16_MAX + i);
            Delay_us(125);
        }
    }
}

/*------------------------------------------------------------------------------
 * CheckHBridgeOverload
 *
 *    Checks whether any of the ClearCore's H-Bridge connectors are experiencing
 *    an overload. If an overload is detected the H-Bridge connectors are reset.
 *
 * Parameters:
 *    None
 *
 * Returns: None
 */
void CheckHBridgeOverload() {
    if (StatusMgr.StatusRT().bit.HBridgeOverloaded) {
        StatusMgr.HBridgeReset();
        Delay_ms(10);
    }
}
//------------------------------------------------------------------------------
