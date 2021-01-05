/*
 * Title: WriteXBeeOutput
 *
 * Objective:
 *    This example demonstrates how to write data to an XBee device connected to
 *    the ClearCore's XBee port.
 *
 * Description:   
 *    This example sets up the XBee connector and writes data to the XBee 
 *    device. Any data received is then read-in and written to the USB serial 
 *    port.
 *
 * Requirements:
 * ** An XBee device connected to ClearCore's XBee connector. Use the XBee in 
 *    Transparent Mode to simply send and receive data without packet structuring.
 * ** Another remote XBee, also in transparent mode and configured to communicate 
 *    with the first.  
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 *
 * Last Modified: 1/21/2020
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"

// ClearCore provides three separate serial interfaces to send communications.
// The three options are ConnectorUsb, ConnectorCOM0, or ConnectorCOM1
#define SerialPort ConnectorUsb

// Select the baud rate to match the target device.
#define serialBaudRate 9600

int main() {
    // Put your setup code here, it will run once:

    // Set up serial communication to print.
    // A port must be open to continue and view incoming data.
    // Set up serial communication to print.
    SerialPort.Mode(Connector::USB_CDC);
    SerialPort.Speed(serialBaudRate);
    SerialPort.PortOpen();
    while (!SerialPort) {
        continue;
    }

    //Uncomment the line below if you have turned RTS flow control on in the XBee's parameters
    //XBee.FlowControl(true);

    // Set the XBee communication speed and open the XBee port
    XBee.Speed(115200);
    XBee.PortOpen();

    // Send a message to the XBee.
    XBee.Send("Hello XBee");

    while (true) {
        // As long as there are characters to be read-in, print the character at the
        // top of the receive buffer.
        if (XBee.AvailableForRead()) {
            SerialPort.SendLine(XBee.CharGet());
        }
    }
}
