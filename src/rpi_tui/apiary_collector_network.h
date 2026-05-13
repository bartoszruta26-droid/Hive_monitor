/**
 * apiary_collector_network.h
 * Moduł sieciowy dla kolektora uli - UDP nasłuchiwanie i HTTP API
 * 
 * AUTOR: ApiaryGuard Pro Team
 * LICENCJA: MIT
 */

#ifndef APIARY_COLLECTOR_NETWORK_H
#define APIARY_COLLECTOR_NETWORK_H

#include "apiary_collector_types.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>

namespace apiary {
namespace collector {

/**
 * Klasa zarządzająca komunikacją sieciową kolektora
 */
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();
    
    /**
     * Inicjalizuje socket UDP do nasłuchiwania danych z Pico/Nano
     * @param port Port nasłuchiwania (domyślnie 5005)
     * @return true jeśli inicjalizacja powiodła się
     */
    bool initUDP(int port = 5005);
    
    /**
     * Inicjalizuje serwer HTTP API
     * @param port Port HTTP (domyślnie 8080)
     * @return true jeśli inicjalizacja powiodła się
     */
    bool initHTTP(int port = 8080);
    
    /**
     * Uruchamia pętlę odbierania danych UDP
     * @param running Flaga kontrolująca działanie pętli
     */
    void startUDPLoop(std::atomic<bool>& running);
    
    /**
     * Uruchamia pętlę serwera HTTP
     * @param running Flaga kontrolująca działanie pętli
     * @param getDataCallback Callback do pobierania danych hive
     */
    void startHTTPLoop(std::atomic<bool>& running,
                       std::function<std::string()> getDataCallback);
    
    /**
     * Zatrzymuje wszystkie wątki i zamyka sockety
     */
    void stop();
    
    /**
     * Przetwarza odebrane dane
     * @param raw_data Surowe dane do przetworzenia
     * @param source_ip Adres IP źródła
     */
    void processData(const std::string& raw_data, const std::string& source_ip);
    
private:
    int udp_socket_;
    int http_socket_;
    std::vector<std::thread> threads_;
};

} // namespace collector
} // namespace apiary

#endif // APIARY_COLLECTOR_NETWORK_H
