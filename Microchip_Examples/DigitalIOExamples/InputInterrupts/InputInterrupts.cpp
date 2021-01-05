/*
 * Title: InputInterrupts
 *
 * Objective:
 *    This example demonstrates how to configure a digital interrupt on a
 *    ClearCore input.
 *
 * Description:
 *    This example sets up and attaches a callback function to be triggered by
 *    a digital interrupt. Interrupts are useful when a function needs to be
 *    called asynchronously from the rest of the code's execution.
 *
 *    This example's interrupt blinks the on-board user LED when the interrupt
 *    pin goes from on to off (aka "falling"). You may notice multiple blinks
 *    depending on how much bounce the input device has.
 *
 *    The interrupt callback function's ability to run is turned on and off
 *    periodically by this example. The callback function can only run when
 *    interrupts are turned "on", regardless of the interrupt pin state. If the
 *    interrupt pin is triggered while interrupts are "off", the callback will
 *    execute when interrupts are next turned on. This on/off state is printed
 *    to the USB serial port.
 *
 * Requirements:
 * ** A digital signal source, such as a switch or sensor, connected to DI-6 to
 *    trigger the interrupt
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

// Connectors that support digital interrupts are:
// DI-6, DI-7, DI-8, A-9, A-10, A-11, A-12
#define interruptConnector ConnectorDI6

// Select the baud rate to match the target serial device
#define baudRate 9600

// Specify the serial connector to use: ConnectorUsb, ConnectorCOM0, or
// ConnectorCOM1
#define SerialPort ConnectorUsb

// Declare the signature for our interrupt service routine (ISR). The function
// is defined below
void MyCallback();

int main() {
    // Set up the interrupt connector in digital input mode.
    interruptConnector.Mode(Connector::INPUT_DIGITAL);

    // Set an ISR to be called when the state of the interrupt pin goes from
    // true to false.
    interruptConnector.InterruptHandlerSet(MyCallback, InputManager::FALLING,
                                           false);

    // Set up serial communication and wait up to 5 seconds for a port to open
    // Serial communication is not required for this example to run.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(baudRate);
    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    SerialPort.PortOpen();
    while (!SerialPort && Milliseconds() - startTime < timeout) {
        continue;
    }

    while (true) {
        // Enable digital interrupts.
        interruptConnector.InterruptEnable(true);

        SerialPort.SendLine("Interrupts are turned on.");

        // Test that the ISR is triggered when the state of the interrupt connector
        // transitions from true to false by toggling your switch.

        // Wait while the interrupt is triggered.
        Delay_ms(5000);

        // Disable digital interrupts.
        interruptConnector.InterruptEnable(false);

        SerialPort.SendLine("Interrupts are turned off.");

        // Test that the ISR does not get triggered when the state of the interrupt
        // connector transitions from true to false by toggling your switch.
        Delay_ms(5000);
    }
}

// The function to be triggered on an interrupt.
// This function blinks the user-controlled LED once.
/*------------------------------------------------------------------------------
 * MyCallback
 *
 *    Flashes the ClearCore's built-in LED (next to the USB port) on and off.
 *
 * Parameters:
 *    None
 *
 * Returns: None
 */
void MyCallback() {
    ConnectorLed.State(true);
    Delay_ms(100);
    ConnectorLed.State(false);
    Delay_ms(100);
}
//------------------------------------------------------------------------------
