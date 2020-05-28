/*
 * Title: WriteAnalogCurrentOutput
 *
 * Objective:
 *    This example demonstrates how to write analog current values to an analog
 *    current output connector.
 *
 * Description:
 *    This example configures pin IO-0 as an analog current output. It outputs
 *    a repeating analog signal, starting at 0mA, increasing to 20mA, and
 *    decreasing back to 0mA.
 *
 * Requirements:
 * ** Connect a device to IO-0 which takes in analog current.
 *
 * Links:
 * ** web link to doxygen (all Examples)
 * ** web link to ClearCore Manual (all Examples)  <<FUTURE links to Getting started webpage/ ClearCore videos>>
 *
 * Last Modified: 1/21/2020
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"


void setup() {
    // Set up connector IO-0 in analog output mode.
    // Note that only connector IO-0 is capable of analog current output.
    ConnectorIO0.Mode(Connector::OUTPUT_ANALOG);
}

void loop() {
    // Ramp the current output of IO-0 up to 20 mA (20,000 uA). If using an
    // operating range of 4-20 mA, change the lower bounds of the loops below
    // to 4000 instead of 0.
    for (uint16_t microAmps = 0; microAmps < 20000; microAmps += 10) {
        ConnectorIO0.OutputCurrent(microAmps);
        Delay_ms(2);
    }

    // Ramp the current output of IO-0 back down.
    for (uint16_t microAmps = 20000; microAmps > 0; microAmps -= 10) {
        ConnectorIO0.OutputCurrent(microAmps);
        Delay_ms(2);
    }
}