/*
 * Title: EthernetTcpClient  
 *
 * Objective:
 *    This example demonstrates how to configure a ClearCore as a TCP client to 
 *    send and receive TCP datagrams (packets).
 *
 * Description:
 *    This example configures a ClearCore device to act as a TCP client. This client 
 *    connects to another device acting as a TCP server to exchange data over 
 *    ethernet TCP. This simple example connects to a server, sends a simple "Hello server"
 *	  message, and receives and prints incoming data from the server.
 *
 *    Partner projects, EthernetTcpServer_autoClientManagement and EthernetTcpServer_manualClientManagement 
 *    are available to configure another ClearCore to act as a server.
 *
 * Setup:
 * 1. Identify the IP address of the server and specify it (as serverIp) below. When 
 *    using either of the EthernetTcpServer examples, the server's IP address will print to a 
 *    connected serial terminal upon startup.
 * 2. Set the usingDhcp boolean as appropriate. If not using DHCP, specify static 
 *    IP address and network information.
 * 3. The server and client must be connected to the same network.
 *    If server and client devices are connected to each other directly (as opposed to connection through a switch)
 *	  an ethernet crossover cable may be required.  
 * 4. It may be helpful to use a terminal application such as PuTTY to view serial output from each device. https://www.putty.org/
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * 
 * Copyright (c) 2022 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */ 

 
#include "ClearCore.h"
#include "EthernetTcpClient.h"
#include "SysTiming.h"

// The IP address of the server you want to connect to
IpAddress serverIp = IpAddress(192, 168, 0, 123);

// The port number over which packets will be sent/received
#define PORT_NUM 8888

// The maximum number of characters allowed per incoming packet
#define MAX_PACKET_LENGTH 100
// Buffer for holding received packets.
unsigned char packetReceived[MAX_PACKET_LENGTH];

// Set usingDhcp to false to use user defined network settings
bool usingDhcp = true;


int main(void) {
	// Set up serial communication between ClearCore and PC serial terminal
	ConnectorUsb.Mode(Connector::USB_CDC);
	ConnectorUsb.Speed(9600);
	ConnectorUsb.PortOpen();
	uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    while (!ConnectorUsb && Milliseconds() - startTime < timeout) {
        continue;
    }
	
	// Set connector IO0 as a digital output
	// When IO0 state is true, its associated LED will turn on
	// indicating a successful connection to a server 
	ConnectorIO0.Mode(Connector::OUTPUT_DIGITAL);

	// Make sure the physical link is active before continuing 
	while (!EthernetMgr.PhyLinkActive()) {
		ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
		Delay_ms(1000);
	}
	
	// Configure with an IP address assigned via DHCP
	EthernetMgr.Setup();
	if (usingDhcp) {
		// Use DHCP to configure the local IP address
		bool dhcpSuccess = EthernetMgr.DhcpBegin();
		if (dhcpSuccess) {
			ConnectorUsb.Send("DHCP successfully assigned an IP address: ");
			ConnectorUsb.SendLine(EthernetMgr.LocalIp().StringValue());
		} else {
			ConnectorUsb.SendLine("DHCP configuration was unsuccessful!");
			while (true) {
				// TCP will not work without a configured IP address
				continue;
			}
		}
	} else {
		// Configure with a manually assigned IP address
	
		// Set ClearCore IP address
		IpAddress ip = IpAddress(192, 168, 0, 103);
		EthernetMgr.LocalIp(ip);
		ConnectorUsb.Send("Assigned manual IP address: ");
		ConnectorUsb.SendLine(EthernetMgr.LocalIp().StringValue());
		
		// Optional: set additional network addresses if needed
		
		//IpAddress gateway = IpAddress(192, 168, 1, 1);
		//IpAddress netmask = IpAddress(255, 255, 255, 0);
		//EthernetMgr.GatewayIp(gateway);
		//EthernetMgr.NetmaskIp(netmask);
	}
	
	// Initialize a client object
	// The ClearCore will operate as a TCP client using this object
	EthernetTcpClient client;
	
	// Attempt to connect to a server
	if (!client.Connect(serverIp, PORT_NUM)) {
				ConnectorUsb.SendLine("Failed to connect to server. Retrying...");
			}
	
	// Connect to server, and send/receive packets
	while(true){
		
		// Make sure the physical link is active before continuing
		while (!EthernetMgr.PhyLinkActive()) {
			ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
			Delay_ms(1000);
		}
		
		// Attempt to connect to server
		if(!client.Connected()){
			// Turn off LED if the client is not connected
			ConnectorIO0.State(false);	
			
			uint32_t delay = 1000;
			if (!client.Connect(serverIp, PORT_NUM)) {
				if(Milliseconds() - startTime > delay){
					ConnectorUsb.SendLine("Failed to connect to server. Retrying...");
					startTime = Milliseconds();
				}
			}
		} else {
			// Turn on LED if client is connected
			ConnectorIO0.State(true);
			
			// If connection was successful, send and receive packets
			if( client.Send("Hello server") >0)
			{
				ConnectorUsb.Send("Sent 'Hello server'. Response from server: ");
				bool receivedMessage = false;
				
				// Read any incoming packets from the server over the next second
				uint32_t delay = 1000;
				startTime = Milliseconds();
				while((Milliseconds() - startTime) < delay){
					if (client.Read(packetReceived, MAX_PACKET_LENGTH) > 0)
					{
						receivedMessage = true;
						ConnectorUsb.Send((char *)packetReceived);
						
						// Clear the message buffer for the next iteration of the loop
						for(int i=0; i<MAX_PACKET_LENGTH; i++){
							packetReceived[i]=NULL;
						}
					}
					EthernetMgr.Refresh();
				}
				// If no packets were received, inform the user via serial message
				if (!receivedMessage){
					ConnectorUsb.SendLine("Didn't receive message.");
				}else{
					ConnectorUsb.SendLine();
				}
			}
			else
			{
				client.Close();
			}
		}
		// Perform any necessary periodic ethernet updates
		// Must be called regularly when actively using ethernet
		EthernetMgr.Refresh();
	}
		
}