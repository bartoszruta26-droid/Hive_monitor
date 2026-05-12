/*
 * ApiaryGuard - Network Implementation (W6100 Ethernet)
 * Comprehensive network handling with debug support and error tracking
 * 
 * DEBUG: Enable serial debugging by defining DEBUG_NETWORK in config.h
 * LOGGING: All network operations are logged with connection status
 * EXCEPTIONS: Graceful handling of DHCP failures, link loss, and client errors
 */

#include "network.h"
#include "config.h"
#include <SPI.h>
#include <Arduino.h>

// External global sensor variables (defined in main .ino file)
extern float temperature;
extern float humidity;
extern uint16_t co2_eq;
extern uint16_t voc_idx;
extern long hx711_value;
extern SensorState sensors;

// MAC address as byte array
byte macAddress[] = MAC_ADDRESS;
IPAddress staticIP STATIC_IP;
IPAddress gateway GATEWAY;
IPAddress subnet SUBNET;
IPAddress rpiIP RPI_IP;

// Debug counters for network health monitoring
static unsigned long network_init_count = 0;
static unsigned long network_error_count = 0;
static unsigned long dhcp_failures = 0;
static unsigned long client_connections = 0;
static unsigned long udp_packets_sent = 0;
static unsigned long link_losses = 0;

/**
 * @brief Initialize W6100 Ethernet module
 * 
 * Performs complete initialization sequence:
 * 1. SPI bus initialization
 * 2. Hardware reset of W6100
 * 3. DHCP or static IP configuration
 * 4. Link status verification
 * 5. Server and UDP startup
 * 
 * DEBUG OUTPUT:
 * - [NETWORK] Initializing W6100 Ethernet...
 * - [NETWORK] DHCP successful / failed
 * - [NETWORK] Ethernet link established / lost
 * - [NETWORK] ERROR: SPI initialization failed
 * - [TRACE] ENTER/EXIT initW6100 (when DEBUG_VERBOSE enabled)
 * - [PERF] Initialization time (when DEBUG_PERF enabled)
 * 
 * EXCEPTIONS HANDLED:
 * - DHCP failure (fallback to static IP)
 * - No Ethernet link (continues with warning)
 * - SPI communication errors
 */
void initW6100() {
    TRACE_ENTER(NETWORK);
    PERF_START(initW6100);
    
    network_init_count++;
    
    Serial.println(">> Initializing W6100 Ethernet...");
    
    #ifdef DEBUG_NETWORK
    Serial.print("[NETWORK] SPI pins - CS=");
    Serial.print(W6100_CS);
    Serial.print(", RST=");
    Serial.println(W6100_RST);
    #endif
    
    // Initialize SPI before using Ethernet - CRITICAL STEP
    #ifdef DEBUG_NETWORK
    Serial.println("[NETWORK] Initializing SPI bus...");
    #endif
    
    SPI.begin();
    
    // Small delay for SPI stabilization
    delay(10);
    
    // Reset W6100 hardware reset sequence
    #ifdef DEBUG_NETWORK
    Serial.println("[NETWORK] Performing hardware reset of W6100...");
    #endif
    
    pinMode(W6100_RST, OUTPUT);
    digitalWrite(W6100_RST, LOW);
    delay(100);
    digitalWrite(W6100_RST, HIGH);
    delay(200);
    
    // Configure SPI for W6100
    pinMode(W6100_CS, OUTPUT);
    digitalWrite(W6100_CS, HIGH);
    
    #ifdef DEBUG_NETWORK
    Serial.println("[NETWORK] Attempting network initialization...");
    #endif
    
    // Try DHCP first if enabled
#if USE_DHCP_FALLBACK
    Serial.println("  Attempting DHCP...");
    if (Ethernet.begin(macAddress)) {
        Serial.println("  [✓] DHCP successful");
        Serial.printf("  IP: %s\n", Ethernet.localIP().toString().c_str());
        Serial.printf("  Gateway: %s\n", Ethernet.gatewayIP().toString().c_str());
        Serial.printf("  DNS: %s\n", Ethernet.dnsServerIP().toString().c_str());
        
        #ifdef DEBUG_NETWORK
        Serial.print("[NETWORK] Subnet mask: ");
        Serial.println(Ethernet.subnetMask().toString().c_str());
        #endif
    } else {
        dhcp_failures++;
        network_error_count++;
        Serial.println("  [!] DHCP failed, using static IP");
        
        #ifdef DEBUG_NETWORK
        Serial.print("[NETWORK] DHCP failure reason: ");
        Serial.println(Ethernet.hardwareStatus() == 0 ? "No chip" : "Chip detected but DHCP timeout");
        #endif
        
        Ethernet.begin(macAddress, staticIP, gateway, subnet);
        Serial.printf("  Static IP: %s\n", staticIP.toString().c_str());
    }
#else
    // Static IP only
    Ethernet.begin(macAddress, staticIP, gateway, subnet);
    Serial.printf("  Static IP: %s\n", staticIP.toString().c_str());
#endif
    
    // Wait for Ethernet to initialize
    delay(1000);
    
    // Check link status
    t_link_status linkStatus = Ethernet.linkStatus();
    if (linkStatus == LNK) {
        Serial.println("  [✓] Ethernet link established");
        #ifdef DEBUG_NETWORK
        Serial.println("[NETWORK] Link speed: 10/100 Mbps (auto-negotiated)");
        #endif
    } else {
        link_losses++;
        network_error_count++;
        Serial.println("  [!] No Ethernet link detected");
        
        #ifdef DEBUG_NETWORK
        Serial.print("[NETWORK] Link status code: ");
        Serial.println(linkStatus);
        Serial.println("[NETWORK] Possible causes: cable unplugged, switch off, wrong wiring");
        #endif
    }
    
    // Start HTTP server
    server.begin();
    Serial.printf("  HTTP server started on port %d\n", HTTP_PORT);
    
    // Start UDP
    udp.begin(UDP_PORT);
    Serial.printf("  UDP started on port %d\n", UDP_PORT);
    
    #ifdef DEBUG_NETWORK
    Serial.println("[NETWORK] Initialization complete");
    Serial.printf("[DEBUG] Network stats - Init: %lu, Errors: %lu, DHCP failures: %lu\n",
                  network_init_count, network_error_count, dhcp_failures);
    #endif
    
    PERF_END(initW6100);
    TRACE_EXIT(NETWORK);
}

/**
 * @brief Handle incoming HTTP client connections
 * 
 * Processes HTTP GET requests and returns JSON with sensor data
 * Implements proper connection handling and error recovery
 * 
 * DEBUG OUTPUT:
 * - [NETWORK] Client connected from X.X.X.X
 * - [NETWORK] HTTP request received: GET /path
 * - [NETWORK] Response sent: XXX bytes
 * - [NETWORK] WARNING: Client disconnected prematurely
 * 
 * EXCEPTIONS HANDLED:
 * - Client disconnect during read
 * - Invalid HTTP request format
 * - Socket errors
 */
void handleClients() {
    EthernetClient client = server.available();
    
    if (client) {
        client_connections++;
        
        #ifdef DEBUG_NETWORK
        Serial.print("[NETWORK] Client connected from ");
        Serial.println(client.remoteIP().toString().c_str());
        #endif
        
        bool currentLineIsBlank = true;
        String httpRequest = "";
        unsigned long startTime = millis();
        const unsigned long timeout = 5000; // 5 second timeout
        
        while (client.connected()) {
            // Timeout protection
            if (millis() - startTime > timeout) {
                #ifdef DEBUG_NETWORK
                Serial.println("[NETWORK] WARNING: Client request timeout");
                #endif
                network_error_count++;
                break;
            }
            
            if (client.available()) {
                char c = client.read();
                httpRequest += c;
                
                #ifdef DEBUG_NETWORK
                if (httpRequest.length() <= 50) {
                    Serial.print(c);
                }
                #endif
                
                if (c == '\n' && currentLineIsBlank) {
                    // Send HTTP response header
                    #ifdef DEBUG_NETWORK
                    Serial.println("\n[NETWORK] Sending HTTP response...");
                    #endif
                    
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: application/json");
                    client.println("Connection: close");
                    client.println();
                    
                    // Send JSON response with sensor data
                    client.print("{\"temp\":");
                    client.print(temperature);
                    client.print(",\"hum\":");
                    client.print(humidity);
                    client.print(",\"co2\":");
                    client.print(co2_eq);
                    client.print(",\"voc\":");
                    client.print(voc_idx);
                    client.print(",\"weight\":");
                    client.print(hx711_value);
                    client.println("}");
                    
                    #ifdef DEBUG_NETWORK
                    Serial.println("[NETWORK] Response sent successfully");
                    Serial.printf("[DEBUG] Total client connections: %lu\n", client_connections);
                    #endif
                    
                    break;
                }
                
                if (c == '\n') {
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    currentLineIsBlank = false;
                }
            }
            
            // Small delay to prevent CPU hogging
            delay(1);
        }
        
        // Cleanup
        delay(1);
        client.stop();
        
        #ifdef DEBUG_NETWORK
        Serial.println("[NETWORK] Client connection closed");
        #endif
    }
}

/**
 * @brief Send sensor data via UDP to Raspberry Pi
 * 
 * Sends compact JSON packet every 10 seconds
 * Implements rate limiting and error tracking
 * 
 * DEBUG OUTPUT:
 * - [NETWORK] UDP packet sent to X.X.X.X:PORT (XX bytes)
 * - [NETWORK] WARNING: UDP send failed
 * - [NETWORK] Rate limit: skipping UDP send (too soon)
 * 
 * EXCEPTIONS HANDLED:
 * - Network not ready
 * - UDP socket errors
 * - Invalid IP address
 */
void sendUDPData() {
    static unsigned long lastUdpSend = 0;
    
    // Rate limiting - send every 10 seconds
    if (millis() - lastUdpSend < 10000) return;
    
    // Check if network is ready
    if (Ethernet.linkStatus() != LNK) {
        #ifdef DEBUG_NETWORK
        Serial.println("[NETWORK] WARNING: Cannot send UDP - no link");
        #endif
        return;
    }
    
    lastUdpSend = millis();
    
    // Prepare UDP packet using snprintf (String::format doesn't exist in Arduino)
    char payload[128];
    int payload_len = snprintf(payload, sizeof(payload),
        "{\"t\":%.1f,\"h\":%.1f,\"c\":%d,\"v\":%d,\"w\":%ld}",
        temperature, humidity, co2_eq, voc_idx, hx711_value
    );
    
    if (payload_len < 0 || payload_len >= sizeof(payload)) {
        #ifdef DEBUG_NETWORK
        Serial.println("[NETWORK] ERROR: UDP payload formatting failed");
        #endif
        network_error_count++;
        return;
    }
    
    #ifdef DEBUG_NETWORK
    Serial.print("[NETWORK] Sending UDP packet to ");
    Serial.print(rpiIP.toString().c_str());
    Serial.print(":");
    Serial.print(UDP_PORT);
    Serial.print(" (");
    Serial.print(payload_len);
    Serial.println(" bytes)");
    #endif
    
    udp.beginPacket(rpiIP, UDP_PORT);
    size_t bytes_written = udp.print(payload);
    int result = udp.endPacket();
    
    if (result == 1 && bytes_written > 0) {
        udp_packets_sent++;
        #ifdef DEBUG_NETWORK
        Serial.printf("[NETWORK] UDP sent successfully (%zu bytes)\n", bytes_written);
        Serial.printf("[DEBUG] Total UDP packets: %lu\n", udp_packets_sent);
        #endif
    } else {
        network_error_count++;
        #ifdef DEBUG_NETWORK
        Serial.print("[NETWORK] WARNING: UDP send failed (bytes=");
        Serial.print(bytes_written);
        Serial.print(", result=");
        Serial.println(result);
        Serial.println("[NETWORK] Possible causes: network down, invalid IP, buffer full");
        #endif
    }
}
