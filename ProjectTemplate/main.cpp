/*
 * Title: SDCardWAVPlayTest
 *
 * Objective:
 *    This example demonstrates how to play .wav files from the SD card
 *
 * Description:
 *    This example plays a "Ring01.wav" file from the SD card through the IO5 connector.
 *
 * Requirements:
 * ** A USB serial connection to the a ClearCore, A micro SD card inserted into the ClearCore's SD card reader, 
 * ** a passive speaker connected to IO5, and a WAV file named "Ring01.wav" loaded on to the micro SD card.
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

#define SerialPort ConnectorUsb

// namespace ClearCore{
//     extern SDManager SDMgr;
// }

// SD chip select pin
const uint8_t chipSelect = CLEARCORE_PIN_INVALID;

//==============================================================================
// End of configuration constants.
//------------------------------------------------------------------------------

int main() {
	//Set up serial communication at a baud rate of 9600 bps then wait up to
	//5 seconds for a port to open.
	//ConnectorUsb communication is not required for this example to run.
	ConnectorUsb.Mode(Connector::USB_CDC);
	ConnectorUsb.Speed(9600);
	uint32_t timeout = 5000;
	uint32_t startTime = Milliseconds();
    ConnectorIO0.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO1.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO2.Mode(Connector::OUTPUT_DIGITAL);
    ConnectorIO3.Mode(Connector::OUTPUT_DIGITAL);
	ConnectorUsb.PortOpen();
	while (!ConnectorUsb && Milliseconds() - startTime < timeout) {
		continue;
	}
    SDManager SDMgr;
	Delay_ms(1000);
	SerialPort.SendLine("Initializing SD card...");
	
	if (!SDMgr.Initialize()) {
		SerialPort.SendLine("initialization failed!");
		return 0;
	}
	SerialPort.SendLine("initialization done.");
    bool outputState = false;
	//Once the SD card is initialized we can play any 8-bit or 16-bit .wav
	// file already loaded on to the SD card:
    SDMgr.Play(50,ConnectorIO4,"Ring01.wav");
    while(!SDMgr.PlaybackFinished()){
        if (outputState) {
            ConnectorIO0.State(true);
            ConnectorIO1.State(true);
            ConnectorIO2.State(true);
            ConnectorIO3.State(true);
        }
        else {
            ConnectorIO0.State(false);
            ConnectorIO1.State(false);
            ConnectorIO2.State(false);
            ConnectorIO3.State(false);
        }
        // Toggle the state to write in the next loop.
        outputState = !outputState;

        // Wait a second, then repeat.
        Delay_ms(1000);
    }
    SDMgr.Play(50,ConnectorIO4,"starlit sands.wav");
    while(!SDMgr.PlaybackFinished()){
        if (outputState) {
            ConnectorIO0.State(true);
            ConnectorIO1.State(true);
            ConnectorIO2.State(true);
            ConnectorIO3.State(true);
        }
        else {
            ConnectorIO0.State(false);
            ConnectorIO1.State(false);
            ConnectorIO2.State(false);
            ConnectorIO3.State(false);
        }
        // Toggle the state to write in the next loop.
        outputState = !outputState;

        // Wait a second, then repeat.
        Delay_ms(1000);
    }
	//Connectors IO4 and IO5 are the two connectors able to drive a speaker
	return 0;
    // 	SD.playFile("Windows XP Critical Stop.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Ding.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Error.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Exclamation.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Hardware Fail.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Hardware Insert.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Hardware Remove.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Logoff Sound.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Logon Sound.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Shutdown.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Shutdown_48.wav",24,ConnectorIO5);
    // 	SD.playFile("Windows XP Startup.wav",50,ConnectorIO5);
    // 	SD.playFile("Windows XP Startup_48.wav",50,ConnectorIO5);
    // 	SD.playFile("Donald Trumps America.wav",50,ConnectorIO5);
    //  SD.playFile("starlit sands.wav",10,ConnectorIO5);

}
