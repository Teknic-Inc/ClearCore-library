/*
 * Title: EthernetTCPServer_autoClientManagement
 *
 * Objective:
 *    This example demonstrates how to configure a ClearCore as a TCP server to 
 *    send and receive TCP datagrams (packets). 
 *    
 * Description:
 *    This example configures a ClearCore device to act as a TCP server. 
 *    This server can receive connections from another device acting as a TCP 
 *    client to exchange data over ethernet TCP. 
 *    This simple example accepts connection requests from clients, receives and
 *    prints incoming data from connected devices, and sends a simple "Hello 
 *    client" response.
 *    A partner project, EthernetTcpClientHelloWorld, is available to configure 
 *    another ClearCore as a client.
 *
 * Setup:
 * 1. Set the usingDhcp boolean as appropriate. If not using DHCP, specify static 
 *    IP and network information.
 * 2. Ensure the server and client are set up to communicate on the same network.
 *    If both devices are directly connected (as opposed to connected through a switch) an ethernet crossover cable may be required. 												  
 * 3. It may be helpful to use a terminal application such as PuTTY to view serial output from each device. https://www.putty.org/
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 * 
 * Copyright (c) 2022 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"
#include "EthernetTcpServer.h"

// The port number on the server over which packets will be sent/received 
#define PORT_NUM 8888

// The maximum number of characters to receive from an incoming packet
#define MAX_PACKET_LENGTH 100
// Buffer for holding received packets
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
	// When IO0 state is true, a LED will light on the
	// ClearCore indicating a successful connection to a client
	ConnectorIO0.Mode(Connector::OUTPUT_DIGITAL);
	
	// Make sure the physical link is active before continuing
	while (!EthernetMgr.PhyLinkActive()) {
		ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
		Delay_ms(1000);
	}
	
	// To configure with an IP address assigned via DHCP
	EthernetMgr.Setup();
	if (usingDhcp) {
		// Use DHCP to configure the local IP address
		
		bool dhcpSuccess = EthernetMgr.DhcpBegin();
		if (dhcpSuccess) {
			ConnectorUsb.Send("DHCP successfully assigned an IP address: ");
			ConnectorUsb.SendLine(EthernetMgr.LocalIp().StringValue());
		}
		else {
			ConnectorUsb.SendLine("DHCP configuration was unsuccessful!");
			while (true) {
				// TCP will not work without a configured IP address
				continue;
			}
		}
	} else {
		// Configure with a manually assigned IP address
		
		// Set ClearCore's IP address
		IpAddress ip = IpAddress(192, 168, 0, 109);
		EthernetMgr.LocalIp(ip);
		ConnectorUsb.Send("Assigned manual IP address: ");
		ConnectorUsb.SendLine(EthernetMgr.LocalIp().StringValue());
		
		// Optionally set additional network addresses if needed
		
		//IpAddress gateway = IpAddress(192, 168, 1, 1);
		//IpAddress netmask = IpAddress(255, 255, 255, 0);
		//EthernetMgr.GatewayIp(gateway);
		//EthernetMgr.NetmaskIp(netmask);
	}
	

	// Initialize the ClearCore as a server that will listen for  
	// incoming client connections on specified port (8888 by default)
	EthernetTcpServer server = EthernetTcpServer(PORT_NUM);
	
	// Initialize a client object
	// This object will hold a connected client's information 
	// allowing the server to interact with the client 
	EthernetTcpClient client;
	
	// Start listening for TCP connections
	server.Begin();

	ConnectorUsb.SendLine("Server now listening for client connections...");

	// Connect to clients, and send/receive packets
	while(true){
		// Obtain a reference to a connected client with incoming data available
		// This function will only return a valid reference if the connected device
		// has data available to read
		client = server.Available();
		
		// Check if the server has returned a connected client with incoming data available
		if (client.Connected() || client.BytesAvailable() > 0) {
			// Flash on LED if a client has sent a message
			ConnectorIO0.State(true);
			
			// Delay to allow user to see the LED
			// This example will flash the LED each time a message from a client is received
			Delay_ms(100); 
			
			// Read packet from the client
			ConnectorUsb.Send("Read the following from the client: ");
			while (client.BytesAvailable() > 0) {
				// Send the data received from the client over a serial port
				client.Read(packetReceived, MAX_PACKET_LENGTH);
				ConnectorUsb.Send((char *)packetReceived);
				
				// Clear the message buffer for the next iteration of the loop
				for(int i=0; i<MAX_PACKET_LENGTH; i++){
					packetReceived[i]= NULL;
				}
			}
			ConnectorUsb.SendLine();
			
			// Send response message to client
			if (client.Send("Hello client ")>0){
				ConnectorUsb.SendLine("Sent 'Hello Client' response");
			}
			else{
				ConnectorUsb.SendLine("Unable to send reply");
			}
		} else{
			// Turn off LED if a message has not been received
			ConnectorIO0.State(false);
			if(client.ConnectionState()->state == CLOSING){
				client.Close();
			}
   
			// Make sure the physical link is active before continuing
			while (!EthernetMgr.PhyLinkActive()) {
				ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
				Delay_ms(1000);
			}
		}
		// Broadcast message to all clients
		//server.Send("Hello all clients ");
		
		// Perform any necessary periodic ethernet updates
		// Must be called regularly when actively using ethernet
		EthernetMgr.Refresh();
	}
	
}
