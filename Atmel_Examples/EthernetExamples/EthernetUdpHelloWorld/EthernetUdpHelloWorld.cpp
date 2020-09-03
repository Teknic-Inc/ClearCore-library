/*
 * Title: EthernetUdpHelloWorld
 *
 * Objective:
 *    This example demonstrates how to enable Ethernet functionality to send and
 *    receive UDP datagrams (packets).
 *
 * Description:
 *    This example will set up ethernet communications between a ClearCore and
 *    another Ethernet source (a PC or ClearCore). The example then prints the
 *    contents of the packets received to the specified serial port and sends
 *    a "Hello World" response to the sender.
 *
 * Requirements:
 * ** Setup 1 (ClearCore and a PC): The PC should be running software capable
 *    of sending and receiving UDP packets. PacketSender is highly recommended
 *    as a free, cross-platform software. Configure PacketSender to send a UDP
 *    packet to the ClearCore by specifying the IP address and port provided to
 *    EthernetMgr.LocalIp(). Your firewall or network settings may need to be
 *    adjusted in order to receive the response back from the ClearCore.
 * ** Setup 2 (ClearCore to a ClearCore): A partner sketch is included at the
 *    end of this file that can be used on the other ClearCore. The MAC address
 *    and IP address values set up for each ClearCore must be unique. The remote
 *    IP address and port used in the partner sketch must match the IP address
 *    and port used to setup the ClearCore in this sketch.
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
#include "EthernetUdp.h"

// Change the IP address below to match your ClearCore's IP address.
IpAddress ip = IpAddress(192, 168, 1, 177);

// The local port to listen for connections on.
uint16_t localPort = 8888;

// The maximum number of characters to receive from an incoming packet
#define MAX_PACKET_LENGTH 100
// Buffer for holding received packets.
unsigned char packetReceived[MAX_PACKET_LENGTH];

// The Ethernet UDP object to let us send and receive packets over UDP
EthernetUdp Udp;

// Set this false if not using DHCP to configure the local IP address.
bool usingDhcp = true;

int main() {
    // Set up serial communication at a baud rate of 9600 bps then wait up to
    // 5 seconds for a port to open.
    // Serial communication is not required for this example to run, however the
    // example will appear to do nothing without serial output.
    ConnectorUsb.Mode(Connector::USB_CDC);
    ConnectorUsb.Speed(9600);
    ConnectorUsb.PortOpen();

    uint32_t timeout = 5000;
    uint32_t startTime = Milliseconds();
    while (!ConnectorUsb && Milliseconds() - startTime < timeout) {
        continue;
    }

    // Make sure the physical link is up before continuing.
    while (!EthernetMgr.PhyLinkActive()) {
        ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
        Delay_ms(1000);
    }

    // Run the setup for the ClearCore Ethernet manager.
    EthernetMgr.Setup();

    if (usingDhcp) {
        // Use DHCP to configure the local IP address.
        bool dhcpSuccess = EthernetMgr.DhcpBegin();
        if (dhcpSuccess) {
            ConnectorUsb.Send("DHCP successfully assigned an IP address: ");
            ConnectorUsb.SendLine(EthernetMgr.LocalIp().StringValue());
        }
        else {
            ConnectorUsb.SendLine("DHCP configuration was unsuccessful!");
            ConnectorUsb.SendLine("Try again using a manual configuration...");
            while (true) {
                // UDP will not work without a configured IP address.
                continue;
            }
        }
    }
    else {
        EthernetMgr.LocalIp(ip);
    }

    // Begin listening on the local port for UDP datagrams
    Udp.Begin(localPort);

    // This loop will wait to receive a packet from a remote source, then reply
    // back with a packet containing a "Hello, world!" message.
    while (true) {
        // Look for a received packet.
        uint16_t packetSize = Udp.PacketParse();

        if (packetSize > 0) {
            ConnectorUsb.Send("Received packet of size ");
            ConnectorUsb.Send(packetSize);
            ConnectorUsb.SendLine(" bytes.");

            ConnectorUsb.Send("Remote IP: ");
            ConnectorUsb.SendLine(Udp.RemoteIp().StringValue());

            ConnectorUsb.Send("Remote port: ");
            ConnectorUsb.SendLine(Udp.RemotePort());

            // Read the packet.
            int32_t bytesRead = Udp.PacketRead(packetReceived, MAX_PACKET_LENGTH);
            ConnectorUsb.Send("Number of bytes read from packet: ");
            ConnectorUsb.SendLine(bytesRead);

            ConnectorUsb.Send("Packet contents: ");
            ConnectorUsb.SendLine((char *)packetReceived);
            ConnectorUsb.SendLine();

            // Send a "Hello, world!" reply packet back to the sender.
            Udp.Connect(Udp.RemoteIp(), Udp.RemotePort());
            Udp.PacketWrite("Hello, world!");
            Udp.PacketSend();
        }

        Delay_ms(10);
    }
}


/*
  // ---------------------------------
  // Partner ClearCore Example Sketch
  // ---------------------------------

#include "ClearCore.h"
#include "EthernetUdp.h"

// Change the IP address below to match this ClearCore's IP address.
IpAddress ip = IpAddress(192, 168, 1, 178);

// The local port to listen for connections on.
uint16_t localPort = 8888;

// The remote ClearCore's IP address and port
IpAddress remoteIp = IpAddress(192, 168, 1, 177);
uint16_t remotePort = 8888;

// The last time you sent a packet to the remote device, in milliseconds.
uint32_t lastSendTime = 0;
// Delay between sending packets, in milliseconds
const uint32_t sendingInterval = 10 * 1000;

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUdp Udp;

// Set this false if not using DHCP to configure the local IP address.
bool usingDhcp = true;

int main() {
    // Set up serial communication at a baud rate of 9600 bps then wait up to
    // 5 seconds for a port to open.
    // Serial communication is not required for this example to run, however the
    // example will appear to do nothing without serial output.
    ConnectorUsb.Mode(Connector::USB_CDC);
    ConnectorUsb.Speed(9600);
    ConnectorUsb.PortOpen();

    // Run the setup for the ClearCore Ethernet manager.
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
            ConnectorUsb.SendLine("Try again using a manual configuration...");
            while (true) {
                // UDP will not work without a configured IP address.
                continue;
            }
        }
    }
    else {
        EthernetMgr.LocalIp(ip);
    }

    // Make sure the physical link is up before continuing.
    while (!EthernetMgr.PhyLinkActive()) {
        ConnectorUsb.SendLine("The Ethernet cable is unplugged...");
        Delay_ms(1000);
    }

    // Begin listening on the local port for UDP datagrams
    Udp.Begin(localPort);

    // This loop will send a packet to the remote IP and port specified every
    // 10 seconds.
    while (true) {
        // Wait for 10 seconds.
        if (Milliseconds() - lastSendTime > sendingInterval) {
            Udp.Connect(remoteIp, remotePort);
            Udp.PacketWrite("Hello ClearCore.");
            Udp.PacketSend();
            lastSendTime = Milliseconds();
        }

        // Keep the connection alive.
        EthernetMgr.Refresh();
    }
}

*/
