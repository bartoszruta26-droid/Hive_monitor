/**
 * apiary_collector.cpp
 * Moduł zbierania danych z wielu uli przez Ethernet (Raspberry Pi 2 + Pico)
 * Kompilacja: g++ -std=c++11 -pthread -o apiary_collector apiary_collector.cpp apiary_logger.cpp
 */

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <map>
#include <sstream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctime>

// Dołączamy loggera i używamy przestrzeni nazw apiary
#include "apiary_logger.cpp"
using namespace apiary;

// Struktura danych z ula
struct HiveData {
    std::string hive_id;
    float temperature;
    float humidity;
    float weight;
    int battery_level;
    long long timestamp;
    bool is_online;
};

// Menadżer danych uli
class ApiaryCollector {
private:
    std::map<std::string, HiveData> hives_data;
    std::mutex data_mutex;
    std::atomic<bool> running{false};
    std::vector<std::thread> worker_threads;
    
    std::vector<std::string> hive_ips; // Lista IP dla uli

    // Symulacja portu nasłuchiwania (w rzeczywistości Pico wysyła dane na ten port)
    int server_socket;
    struct sockaddr_in server_addr;

public:
    ApiaryCollector() {
        server_socket = -1;
        // Inicjalizacja loggera przez getInstance
        Logger::getInstance().initialize(LoggerConfig{});
    }

    ~ApiaryCollector() {
        stop();
    }

    // Konfiguracja listy uli (IP)
    void configureHives(const std::vector<std::string>& ips) {
        hive_ips = ips;
        Logger::getInstance().info( "Skonfigurowano " + std::to_string(ips.size()) + " uli do monitorowania.");
        
        // Inicjalizacja struktury danych
        std::lock_guard<std::mutex> lock(data_mutex);
        for (size_t i = 0; i < ips.size(); ++i) {
            std::string id = "UL-" + std::to_string(i + 1);
            hives_data[id] = {id, 0.0, 0.0, 0.0, 0, 0, false};
            Logger::getInstance().debug( "Dodano ul: " + id + " (IP: " + ips[i] + ")");
        }
    }

    // Inicjalizacja socketu UDP do nasłuchiwania danych z Pico
    bool initNetwork(int port = 5005) {
        server_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (server_socket < 0) {
            Logger::getInstance().error( "Nie udało się utworzyć socketu: " + std::string(strerror(errno)));
            return false;
        }

        // Ustawienie socketu na nieblokujący (opcjonalne, ale dobre dla pętli)
        int flags = fcntl(server_socket, F_GETFL, 0);
        fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // Nasłuchuj na wszystkich interfejsach
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            Logger::getInstance().error( "Błąd bindowania portu " + std::to_string(port) + ": " + strerror(errno));
            close(server_socket);
            return false;
        }

        Logger::getInstance().info( "Serwer nasłuchujący uruchomiony na porcie UDP " + std::to_string(port));
        return true;
    }

    // Główna pętla odbierania danych (wątek)
    void receiveLoop() {
        char buffer[1024];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        while (running) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t recv_len = recvfrom(server_socket, buffer, sizeof(buffer)-1, 0, 
                                        (struct sockaddr*)&client_addr, &client_len);

            if (recv_len > 0) {
                buffer[recv_len] = '\0';
                processData(std::string(buffer), inet_ntoa(client_addr.sin_addr));
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // Prawdziwy błąd sieciowy
                if (running) Logger::getInstance().warning( "Błąd odbioru danych: " + std::string(strerror(errno)));
            }
            
            // Krótka pauza, aby nie obciążać CPU w pętli busy-wait
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    // Parsowanie i przetwarzanie danych
    void processData(const std::string& raw_data, const std::string& source_ip) {
        // Format oczekiwany od Pico: ID,HUM,TEMP,WEIGHT,BAT
        // Przykład: "UL-1,65.5,24.3,45.2,98"
        
        std::stringstream ss(raw_data);
        std::string segment;
        std::vector<std::string> parts;

        while (std::getline(ss, segment, ',')) {
            parts.push_back(segment);
        }

        if (parts.size() < 5) {
            Logger::getInstance().warning( "Niepoprawny format danych z " + source_ip + ": " + raw_data);
            return;
        }

        try {
            std::string hive_id = parts[0];
            float humidity = std::stof(parts[1]);
            float temp = std::stof(parts[2]);
            float weight = std::stof(parts[3]);
            int battery = std::stoi(parts[4]);
            long long now = std::time(nullptr);

            // Aktualizacja danych w pamięci współdzielonej
            {
                std::lock_guard<std::mutex> lock(data_mutex);
                if (hives_data.find(hive_id) != hives_data.end()) {
                    hives_data[hive_id].humidity = humidity;
                    hives_data[hive_id].temperature = temp;
                    hives_data[hive_id].weight = weight;
                    hives_data[hive_id].battery_level = battery;
                    hives_data[hive_id].timestamp = now;
                    hives_data[hive_id].is_online = true;
                    
                    Logger::getInstance().debug( "Zaktualizowano dane dla " + hive_id + 
                               " [T:" + std::to_string(temp) + " H:" + std::to_string(humidity) + "]");
                } else {
                    // Nowy ul dynamicznie wykryty
                    Logger::getInstance().info( "Wykryto nowy ul: " + hive_id + " z IP " + source_ip);
                    HiveData new_hive = {hive_id, temp, humidity, weight, battery, now, true};
                    hives_data[hive_id] = new_hive;
                }
            }
        } catch (const std::exception& e) {
            Logger::getInstance().error( "Błąd parsowania danych: " + std::string(e.what()));
        }
    }

    // Symulacja danych (do testów bez fizycznych Pico)
    void simulationLoop() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            std::lock_guard<std::mutex> lock(data_mutex);
            for (auto& pair : hives_data) {
                // Symuluj zmiany temperatury i wilgotności
                pair.second.temperature += (rand() % 10 - 5) / 10.0f;
                pair.second.humidity += (rand() % 10 - 5) / 10.0f;
                pair.second.timestamp = std::time(nullptr);
                
                // Loguj co pewien czas
                if (rand() % 5 == 0) {
                    Logger::getInstance().info( "Symulacja: " + pair.first + 
                               " Temp: " + std::to_string(pair.second.temperature));
                }
            }
        }
    }

    void start(bool use_simulation = false) {
        if (running) return;
        running = true;

        if (!use_simulation) {
            if (!initNetwork()) {
                Logger::getInstance().warning( "Przełączanie w tryb symulacji z powodu błędu sieci.");
                use_simulation = true;
            }
        }

        if (use_simulation) {
            Logger::getInstance().info( "Uruchamianie symulatora danych uli...");
            worker_threads.emplace_back(&ApiaryCollector::simulationLoop, this);
        } else {
            Logger::getInstance().info( "Uruchamianie nasłuchiwania sieciowego...");
            worker_threads.emplace_back(&ApiaryCollector::receiveLoop, this);
        }
    }

    void stop() {
        running = false;
        for (auto& t : worker_threads) {
            if (t.joinable()) t.join();
        }
        worker_threads.clear();
        if (server_socket >= 0) close(server_socket);
        Logger::getInstance().info( "Kolektor danych zatrzymany.");
    }

    // Metoda dla TUI/Basha do pobrania aktualnego stanu (eksport do JSON lub CSV)
    std::string getStatusJSON() {
        std::lock_guard<std::mutex> lock(data_mutex);
        std::stringstream json;
        json << "{";
        bool first = true;
        for (const auto& pair : hives_data) {
            if (!first) json << ",";
            first = false;
            const auto& d = pair.second;
            json << "\"" << d.hive_id << "\":{"
                 << "\"temp\":" << d.temperature << ","
                 << "\"hum\":" << d.humidity << ","
                 << "\"weight\":" << d.weight << ","
                 << "\"bat\":" << d.battery_level << ","
                 << "\"online\":" << (d.is_online ? "true" : "false") << "}";
        }
        json << "}";
        return json.str();
    }
    
    // Prostszy format dla Bash (CSV)
    std::string getStatusCSV() {
        std::lock_guard<std::mutex> lock(data_mutex);
        std::stringstream csv;
        csv << "ID,STATUS,TEMP,HUM,WIGHT,BAT,TIME\n";
        for (const auto& pair : hives_data) {
            const auto& d = pair.second;
            std::string status = d.is_online ? "ONLINE" : "OFFLINE";
            if (d.is_online && (std::time(nullptr) - d.timestamp > 60)) status = "STALE"; // Brak danych > 60s
            
            csv << d.hive_id << "," 
                << status << "," 
                << d.temperature << "," 
                << d.humidity << "," 
                << d.weight << "," 
                << d.battery_level << ","
                << d.timestamp << "\n";
        }
        return csv.str();
    }
};

// Funkcja główna do samodzielnego uruchomienia jako demon
int main(int argc, char* argv[]) {
    // ApiaryCollector używa singleton Logger
    ApiaryCollector collector;

    // Konfiguracja przykładowych IP uli (w produkcji czytane z config file)
    std::vector<std::string> hives = {"192.168.1.101", "192.168.1.102", "192.168.1.103"};
    collector.configureHives(hives);

    // Uruchomienie w trybie symulacji (demonstracja) lub sieciowym
    bool sim_mode = (argc > 1 && std::string(argv[1]) == "--sim");
    
    collector.start(sim_mode);

    // Pętla główna demona (utrzymuje proces przy życiu)
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // Tutaj można dodać wysyłkę danych do serwera online
    }

    return 0;
}
