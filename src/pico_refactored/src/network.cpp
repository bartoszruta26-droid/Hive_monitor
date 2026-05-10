/*
 * ApiaryGuard - Network Implementation (W6100 Ethernet)
 */

#include "network.h"
#include "config.h"

// External global sensor variables (defined in main)
extern float temperature;
extern float humidity;
extern uint16_t co2_eq;
extern uint16_t voc_idx;
extern long hx711_value;

// MAC address as byte array
byte macAddress[] = MAC_ADDRESS;
IPAddress staticIP STATIC_IP;
IPAddress gateway GATEWAY;
IPAddress subnet SUBNET;
IPAddress rpiIP RPI_IP;

void initW6100() {
    Serial.println(">> Initializing W6100 Ethernet...");
    
    // Reset W6100
    pinMode(W6100_RST, OUTPUT);
    digitalWrite(W6100_RST, LOW);
    delay(100);
    digitalWrite(W6100_RST, HIGH);
    delay(200);
    
    // Configure SPI for W6100
    pinMode(W6100_CS, OUTPUT);
    digitalWrite(W6100_CS, HIGH);
    
    // Try DHCP first if enabled
#if USE_DHCP_FALLBACK
    Serial.println("  Attempting DHCP...");
    if (Ethernet.begin(macAddress)) {
        Serial.println("  [✓] DHCP successful");
        Serial.printf("  IP: %s\n", Ethernet.localIP().toString().c_str());
        Serial.printf("  Gateway: %s\n", Ethernet.gatewayIP().toString().c_str());
    } else {
        Serial.println("  [!] DHCP failed, using static IP");
        Ethernet.begin(macAddress, staticIP, gateway, subnet);
    }
#else
    // Static IP only
    Ethernet.begin(macAddress, staticIP, gateway, subnet);
    Serial.printf("  Static IP: %s\n", staticIP.toString().c_str());
#endif
    
    // Wait for Ethernet to initialize
    delay(1000);
    
    // Check link status
    if (Ethernet.linkStatus() == LNK) {
        Serial.println("  [✓] Ethernet link established");
    } else {
        Serial.println("  [!] No Ethernet link detected");
    }
    
    // Start HTTP server
    server.begin();
    Serial.printf("  HTTP server started on port %d\n", HTTP_PORT);
    
    // Start UDP
    udp.begin(UDP_PORT);
    Serial.printf("  UDP started on port %d\n", UDP_PORT);
}

void handleClients() {
    EthernetClient client = server.available();
    
    if (client) {
        bool currentLineIsBlank = true;
        String httpRequest = "";
        
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                httpRequest += c;
                
                if (c == '\n' && currentLineIsBlank) {
                    // Send HTTP response header
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
                    
                    break;
                }
                
                if (c == '\n') {
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    currentLineIsBlank = false;
                }
            }
        }
        
        delay(1);
        client.stop();
    }
}

void sendUDPData() {
    static unsigned long lastUdpSend = 0;
    
    if (millis() - lastUdpSend < 10000) return; // Send every 10 seconds
    lastUdpSend = millis();
    
    // Prepare UDP packet
    String payload = String::format(
        "{\"t\":%.1f,\"h\":%.1f,\"c\":%d,\"v\":%d,\"w\":%ld}",
        temperature, humidity, co2_eq, voc_idx, hx711_value
    );
    
    udp.beginPacket(rpiIP, UDP_PORT);
    udp.print(payload);
    udp.endPacket();
}
