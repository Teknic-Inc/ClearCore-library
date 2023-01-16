/*
 * Title: EthernetTCPServer_manualClientManagement 
 *
 * Objective:
 *    This example demonstrates how to configure a ClearCore as a TCP server to 
 *    send and receive TCP datagrams (packets).
 *    
 * Description:
 *    This example configures a ClearCore device to act as a TCP server. 
 *    This server can receive connections from several other devices acting as TCP 
 *    clients to exchange data over ethernet TCP. 
 *    This simple example accepts connection requests from clients, checks for
 *    incoming data from connected devices, and sends a simple "Hello 
 *    client" response.
 *    A partner project, EthernetTcpClientHelloWorld, can be used to configure 
 *    another ClearCore as a client device.
 *
 * Setup:
 * 1. Set the usingDhcp boolean as appropriate. If not using DHCP, specify static 
 *    IP and network information.
 * 2. Ensure the server and client are setup to communicate on the same network.
 *    If server and client devices are directly connected (as opposed to through a switch)
 *    an ethernet crossover cable may be required. 
 * 3. It may be helpful to use another application to view serial output from 
 *    each device. PuTTY is one such application: https://www.putty.org/
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
// Buffer for holding packets to send
char packetToSend[MAX_PACKET_LENGTH]; 

// Set total number of clients the server will accept
#define NUMBER_OF_CLIENTS 6

// Set usingDhcp to false to use user defined network settings
bool usingDhcp = true;

// Array of output LEDs to indicate client connections
// NOTE: only connectors IO0 through IO5 can be configured as digital outputs (LEDs) 
Connector *const outputLEDs[6] = {
	&ConnectorIO0, &ConnectorIO1, &ConnectorIO2, &ConnectorIO3, &ConnectorIO4, &ConnectorIO5
};


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

	// Set connectors IO0-IO5 as a digital outputs 
	// When an outputLED's state is true, a LED will light on the
	// ClearCore indicating a successful connection to a client
	for(int i=0; i<6; i++){
	outputLEDs[i]->Mode(Connector::OUTPUT_DIGITAL);
	}
	
	// Make sure the physical link is active before continuing
	while (!EthernetMgr.PhyLinkActive()) {
		ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
		Delay_ms(1000);
	}
	
	//To configure with an IP address assigned via DHCP
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
		IpAddress ip = IpAddress(192, 168, 0, 100);
		EthernetMgr.LocalIp(ip);
		ConnectorUsb.Send("Assigned manual IP address: ");
		ConnectorUsb.SendLine(EthernetMgr.LocalIp().StringValue());
		
		// Optionally, set additional network addresses if needed
		
		//IpAddress gateway = IpAddress(192, 168, 1, 1);
		//IpAddress netmask = IpAddress(255, 255, 255, 0);
		//EthernetMgr.GatewayIp(gateway);
		//EthernetMgr.NetmaskIp(netmask);
	}
	

	// Initialize the ClearCore as a server 
	// Clients connect on specified port (8888 by default)
	EthernetTcpServer server = EthernetTcpServer(PORT_NUM);
	
	// Initialize an array of clients
	// This array holds the information of any client that
	// is currently connected to the server, and is used by the
	// server to interact with these clients
	EthernetTcpClient clients[NUMBER_OF_CLIENTS];
	// Initialize an temporary client for client management
	EthernetTcpClient tempClient;
	
	bool newClient = 0;

	// Start listening for TCP connections
	server.Begin();

	ConnectorUsb.SendLine("Server now listening for client connections...");

	// Resets a timer used to display a list of connected clients
	startTime = 0;
	
	// Connect to clients, and send/receive packets
	while(true){
		
		// Obtain a reference to a connected client
		// This function returns a specific client once per connection attempt
		// To maintain a connection with a client using Accept() 
		// some basic client management must be performed
		tempClient = server.Accept();
		
		// Checks if server.Accept() has returned a new client
		if(tempClient.Connected()){
			newClient = 1; 
			
			for(int i=0; i<NUMBER_OF_CLIENTS; i++){
				//Checks for an available location to store a new client
				if(!clients[i].Connected()){
					clients[i].Close();
					clients[i] = tempClient;
					ConnectorUsb.Send(clients[i].RemoteIp().StringValue());
					ConnectorUsb.SendLine(" has been connected");
					newClient = 0;
					break;
				}
			}
			// Rejects client if the client list is full 
			if(newClient == 1){
				newClient = 0;
				tempClient.Send("This server has reached its max number of clients. Closing connection.");
				sprintf(packetToSend, "This server has reached its max number of clients. Closing connection to (%s).", tempClient.RemoteIp().StringValue());
				ConnectorUsb.SendLine(packetToSend);
				tempClient.Close();
			}
		}
		
		// Loops through list of clients to receive/send messages
		for(int i=0; i<NUMBER_OF_CLIENTS; i++){
			if (clients[i].Connected()) {
				
				// Indicate connection on corresponding output LED
				outputLEDs[i]->State(true);
				
				// Check if client has incoming data available	
				if(clients[i].BytesAvailable()){
					sprintf(packetToSend, "Read the following from the client(%s): ", clients[i].RemoteIp().StringValue());
					ConnectorUsb.Send(packetToSend);
					// Read packet from the client
					while (clients[i].BytesAvailable()) {
						// Send the data received from the client over a serial port
						clients[i].Read(packetReceived, MAX_PACKET_LENGTH);
						ConnectorUsb.Send((char *)packetReceived);

						// Clear the message buffer for the next iteration of the loop
						for(int i=0; i<MAX_PACKET_LENGTH; i++){
							packetReceived[i]=NULL;
						}
					}
					ConnectorUsb.SendLine();
					
					//Sends unique response to the client
					sprintf(packetToSend, "Hello client %s",clients[i].RemoteIp().StringValue());
					if(clients[i].Send(packetToSend) == 0){
						// If the message was unable to send, close the client connection
						clients[i].Close();
						sprintf(packetToSend, "Client (%s) has been removed from client list. ", clients[i].RemoteIp().StringValue());
						ConnectorUsb.Send(packetToSend);
						clients[i] = EthernetTcpClient();
						
						// Indicate loss of connection by turning off corresponding LED
						outputLEDs[i]->State(false);
					}

				}
				
			} else{
				// Removes any disconnected clients
				if(clients[i].RemoteIp() != IpAddress(0,0,0,0)){
					sprintf(packetToSend, "Client (%s) has been removed from client list. ", clients[i].RemoteIp().StringValue());
					ConnectorUsb.Send(packetToSend);
					clients[i].Close();
					clients[i] = EthernetTcpClient();

					// Indicate loss of connection by turning off corresponding LED
					outputLEDs[i]->State(false);
				}
			}
		}
		
		// Make sure the physical link is active before continuing
		while (!EthernetMgr.PhyLinkActive()) {
			ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
			Delay_ms(1000);
		}
		
		// Print out a list of current clients
		int delay = 5000;
		if(Milliseconds()- startTime > delay){
			ConnectorUsb.SendLine("List of current clients: ");
			for(int i=0; i<NUMBER_OF_CLIENTS; i++){
				if(clients[i].RemoteIp() != IpAddress(0,0,0,0)){
					sprintf(packetToSend, "Client %i = %s", i, clients[i].RemoteIp().StringValue());
					ConnectorUsb.SendLine(packetToSend);
				}
			}
			startTime = Milliseconds();
		}
		
		// Perform any necessary periodic ethernet updates
		// Must be called regularly when actively using ethernet
		EthernetMgr.Refresh();
	}
}

