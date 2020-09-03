/*
 * Title: WriteCCIODigitalOutputPulses
 *
 * Objective:
 *    This example demonstrates how to initialize a CCIO-8 Expansion Board and
 *    write digital pulses to its outputs.
 *
 * Description:
 *    This example sets up COM-0 to control a CCIO-8 Expansion Board then
 *    writes a series of digital pulses to the defined connector.
 *
 * Requirements:
 * ** A CCIO-8 Expansion Board powered and connected to COM-0.
 * ** An output such as an LED connected to defined connector (CCIO-0).
 *      Note: You can leave the I/O point disconnected and still see the
 *      built-in I/O LED toggle with the connector state.
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

// Specify which output pin to write digital output pulses.
// All connectors on the ClearCore I/O Expansion Board are capable of
// pulsing the output.
#define OutputPin CcioMgr.PinByIndex(CLEARCORE_PIN_CCIOA0)

int main() {
    // Set up the CCIO-8 COM port.
    CcioPort.Mode(Connector::CCIO);
    CcioPort.PortOpen();

    // Set up the output connector in output mode.
    OutputPin->Mode(Connector::OUTPUT_DIGITAL);

    while (true) {
        // Generate a 100ms on/100ms off pulse that runs until
        // the stop function is called.
        OutputPin->OutputPulsesStart(100, 100);
        Delay_ms(1000);

        // Stop any further pulses on the pin. The second argument controls how
        // output pulses are stopped. If true, the output pulses will be stopped
        // immediately (this is the default behavior); if false, the active
        // pulse cycle will be completed first.
        OutputPin->OutputPulsesStop(true);

        // Generate a 250ms on/50ms off pulse that continues until
        // 20 on/off cycles are complete or until the stop function
        // is called.
        OutputPin->OutputPulsesStart(250, 50, 20);
        Delay_ms(6000);

        // Pulses should be complete, but call stop to be safe.
        OutputPin->OutputPulsesStop(true);

        // Generate a 300ms on/500ms off pulse that runs for 5 complete cycles.
        // With the final true argument, the sketch will pause here until all
        // the pulse cycles are complete.
        OutputPin->OutputPulsesStart(300, 500, 5, true);
        Delay_ms(3000);
    }
}
