/*
 * Title: WritePwmOutput
 *
 * Objective:
 *    This example demonstrates how to write a digital PWM signal to a ClearCore
 *    digital output.
 *
 * Description:
 *    This example sets the defined pin as an output then writes a series of
 *    PWM signals with varying duty cycles to the output.
 *
 * Requirements:
 * ** Connect a device that takes in a PWM signal to IO-1.
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

// Specify which output pin to write digital PWM to.
// PWM-capable pins: IO-0 through IO-5.
// Note: IO-4 and IO-5 are capable of bi-directional and higher-current PWM
//       output using an H-Bridge. See the WriteHBridgeOutput example.
#define outputPin ConnectorIO1

int main() {
    // Set up the output pin for PWM output mode.
    outputPin.Mode(Connector::OUTPUT_PWM);

    while (true) {
        // Write some digital PWM signals to the output connector.
        // Valid values range from 0 (0% duty cycle / always off)
        // to 255 (100% duty cycle / always on).

        // Output a low duty cycle for 1 second.
        outputPin.PwmDuty(10);
        Delay_ms(1000);

        // Output a medium duty cycle for 1 second.
        outputPin.PwmDuty(120);
        Delay_ms(1000);

        // Output a high duty cycle for 1 second.
        outputPin.PwmDuty(230);
        Delay_ms(1000);
    }
}
