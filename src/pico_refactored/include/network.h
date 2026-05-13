/*
 * ApiaryGuard - Network Module (W6100 Ethernet)
 * Handles Ethernet initialization, HTTP server, and UDP communication
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "config.h"

// External network objects
extern EthernetServer server;
extern EthernetUDP udp;

// Function declarations
void initW6100();
void handleClients();
void sendUDPData();

// DHCP fallback configuration
#define USE_DHCP_FALLBACK true

#endif // NETWORK_H
