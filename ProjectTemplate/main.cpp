/*
 * Title: SDCardReadWriteTest
 *
 * Objective:
 *    This example demonstrates how to use the reading and writing functionality
 *    of the ClearCore SD card reader.
 *
 * Description:
 *    This example reads from and writes to a .txt file
 *
 * Requirements:
 * ** A USB serial connection to the a ClearCore, An SD card inserted into the ClearCore's SD card reader
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
#include "SdFat.h"
#include "FatFile.h"

#define SerialPort ConnectorUsb

// SD chip select pin
const uint8_t chipSelect = CLEARCORE_PIN_INVALID;

// File size in MB where MB = 1,000,000 bytes.
const uint32_t FILE_SIZE_MB = 5;

//==============================================================================
// End of configuration constants.
//------------------------------------------------------------------------------

// test file
FatFile file;

int main() {
	//Set up serial communication at a baud rate of 9600 bps then wait up to
	//5 seconds for a port to open.
	//ConnectorUsb communication is not required for this example to run, however the
	//example will appear to do nothing without serial output.
	ConnectorUsb.Mode(Connector::USB_CDC);
	ConnectorUsb.Speed(9600);
	uint32_t timeout = 5000;
	uint32_t startTime = Milliseconds();
	ConnectorUsb.PortOpen();
	while (!ConnectorUsb && Milliseconds() - startTime < timeout) {
		continue;
	}
	Delay_ms(1000);
	SerialPort.SendLine("Initializing SD card...");

	//Initialize SD variables:
	SdFat SD;
	FatFile myFile;
    uint8_t buf[1024];
    for(size_t i = 0; i<sizeof(buf);i++){
        buf[i] = 0;
    }
	
	if (!SD.begin()) {
		SerialPort.SendLine("initialization failed!");
		return 0;
	}
	SerialPort.SendLine("initialization done.");

	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.

	// re-open the file for reading:
	myFile.open("TEST.txt");
	if (myFile.isOpen()) {
		SerialPort.SendLine("TEST.txt:");

		// read from the file until there's nothing else in it:
            myFile.readASync(buf,sizeof(buf));
            Delay_ms(1000);
            for(size_t i = 0; i<sizeof(buf);i++){
                SerialPort.Send((char)buf[i]);
            }
		// close the file:
		myFile.close();
		} else {
		// if the file didn't open, print an error:
		SerialPort.SendLine("error opening test.txt");
	}
	return 0;

}
