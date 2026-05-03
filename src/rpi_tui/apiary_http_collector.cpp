/**
 * apiary_http_collector.cpp
 * Realny kolektor danych z Raspberry Pi Pico przez Ethernet (HTTP API)
 * 
 * Ten program łączy się z rzeczywistym API HTTP serwera na Raspberry Pi Pico
 * i pobiera dane sensoryczne zgodnie z endpointami zdefiniowanymi w pico.ino
 * 
 * Endpointy API (port 8080):
 *   - GET /status              - podstawowe dane: temp, hum, weight, co2, voc
 *   - GET /radar/status        - status radaru MMWave
 *   - GET /radar/anomalies     - anomalie radaru
 *   - GET /hx711/status        - status wagi
 *   - GET /hx711/metrics       - metryki wagi
 *   - GET /hx711/events        - zdarzenia wagi
 *   - GET /audio/status        - status audio
 *   - GET /audio/spectrum      - widmo audio
 *   - GET /airquality/status   - jakość powietrza
 *   - GET /airquality/metrics  - metryki powietrza
 * 
 * Kompilacja:
 *   g++ -std=c++17 -pthread -o apiary_http_collector apiary_http_collector.cpp -lcurl
 * 
 * Uruchomienie:
 *   ./apiary_http_collector --config hives.conf
 *   ./apiary_http_collector --pico 192.168.1.177 --interval 5
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
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Konfiguracja ula
struct HiveConfig {
    std::string hive_id;
    std::string pico_ip;
    int pico_port;
    int poll_interval; // w sekundach
};

// Struktura danych z ula - wszystkie metryki z API Pico
struct HiveData {
    std::string hive_id;
    
    // Podstawowe dane środowiskowe
    float temperature{0.0f};
    float humidity{0.0f};
    float weight{0.0f};
    uint16_t co2_eq{0};
    uint16_t voc_idx{0};
    
    // Dane z wagi HX711
    float hx711_raw{0.0f};
    float hx711_rate{0.0f};
    float honey_production_idx{0.0f};
    float colony_growth_rate{0.0f};
    float predicted_weight_24h{0.0f};
    
    // Dane z radaru MMWave
    int radar_target_count{0};
    float radar_distance{0.0f};
    float hive_health_index{0.0f};
    float activity_ratio{0.0f};
    std::string last_anomaly_type{"NONE"};
    
    // Dane audio
    float audio_rms{0.0f};
    float dominant_frequency{0.0f};
    float bee_activity_index{0.0f};
    float swarm_probability{0.0f};
    
    // Jakość powietrza
    float iaq_index{0.0f};
    int ventilation_need{0};
    
    // Status
    bool is_online{false};
    long long last_update{0};
    long long timestamp{0};
    int error_count{0};
};

// Callback dla CURL - zapis odpowiedzi do stringa
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t total_size = size * nmemb;
    userp->append((char*)contents, total_size);
    return total_size;
}

// Klasa klienta HTTP do komunikacji z Pico
class PicoHttpClient {
private:
    std::string base_url;
    CURL* curl;
    
public:
    PicoHttpClient(const std::string& ip, int port = 8080) {
        base_url = "http://" + ip + ":" + std::to_string(port);
        curl = curl_easy_init();
        
        if (!curl) {
            throw std::runtime_error("Nie udało się zainicjalizować CURL");
        }
        
        // Konfiguracja timeoutów
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }
    
    ~PicoHttpClient() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }
    
    // Pobierz dane z endpointu JSON
    bool getJson(const std::string& endpoint, json& result) {
        std::string response_data;
        std::string url = base_url + endpoint;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            return false;
        }
        
        // Sprawdź kod HTTP
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code != 200) {
            return false;
        }
        
        try {
            result = json::parse(response_data);
            return true;
        } catch (const json::parse_error& e) {
            return false;
        }
    }
    
    // Proste żądanie tekstowe (dla komend sterujących)
    bool getText(const std::string& endpoint, std::string& result) {
        std::string response_data;
        std::string url = base_url + endpoint;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
        
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            return false;
        }
        
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code != 200) {
            return false;
        }
        
        result = response_data;
        return true;
    }
};

// Menadżer zbierania danych z wielu uli
class ApiaryHttpCollector {
private:
    std::map<std::string, HiveData> hives_data;
    std::vector<HiveConfig> hive_configs;
    std::mutex data_mutex;
    std::atomic<bool> running{false};
    std::vector<std::thread> worker_threads;
    
    // Logger prosty
    void log(const std::string& level, const std::string& message) {
        std::time_t now = std::time(nullptr);
        char time_buf[64];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        std::cout << "[" << time_buf << "] [" << level << "] " << message << std::endl;
    }
    
    void info(const std::string& msg) { log("INFO", msg); }
    void warning(const std::string& msg) { log("WARNING", msg); }
    void error(const std::string& msg) { log("ERROR", msg); }
    void debug(const std::string& msg) { log("DEBUG", msg); }
    
    // Wątek zbierający dane z pojedynczego ula
    void collectHiveData(const HiveConfig& config) {
        info("Uruchamianie zbierania danych z ula: " + config.hive_id + 
             " (IP: " + config.pico_ip + ":" + std::to_string(config.pico_port) + ")");
        
        PicoHttpClient client(config.pico_ip, config.pico_port);
        
        while (running) {
            auto start_time = std::chrono::steady_clock::now();
            
            try {
                HiveData data;
                data.hive_id = config.hive_id;
                data.timestamp = std::time(nullptr);
                bool any_success = false;
                
                // 1. Pobierz podstawowy status
                json status_json;
                if (client.getJson("/status", status_json)) {
                    data.temperature = status_json.value("temp", 0.0f);
                    data.humidity = status_json.value("hum", 0.0f);
                    data.weight = status_json.value("weight", 0.0f);
                    data.co2_eq = status_json.value("co2", 0);
                    data.voc_idx = status_json.value("voc", 0);
                    data.is_online = true;
                    data.last_update = std::time(nullptr);
                    data.error_count = 0;
                    any_success = true;
                    
                    debug("[" + config.hive_id + ] Status: T=" + 
                          std::to_string(data.temperature) + "°C, H=" + 
                          std::to_string(data.humidity) + "%");
                } else {
                    data.error_count++;
                    if (data.error_count > 5) {
                        data.is_online = false;
                        warning("[" + config.hive_id + "] Brak odpowiedzi po " + 
                               std::to_string(data.error_count) + " próbach");
                    }
                }
                
                // 2. Pobierz dane z wagi HX711
                json hx711_json;
                if (client.getJson("/hx711/status", hx711_json)) {
                    data.hx711_raw = hx711_json.value("raw_value", 0.0f);
                    data.hx711_rate = hx711_json.value("current_rate", 0.0f);
                    data.honey_production_idx = hx711_json.value("honey_production_idx", 0.0f);
                    data.colony_growth_rate = hx711_json.value("colony_growth_rate", 0.0f);
                    data.predicted_weight_24h = hx711_json.value("predicted_weight_24h", 0.0f);
                    any_success = true;
                }
                
                // 3. Pobierz dane z radaru MMWave
                json radar_json;
                if (client.getJson("/radar/status", radar_json)) {
                    data.radar_target_count = radar_json.value("radar_data_count", 0);
                    data.radar_distance = radar_json.value("last_distance", 0.0f);
                    data.hive_health_index = radar_json.value("hive_health_index", 0.0f);
                    data.activity_ratio = radar_json.value("activity_ratio", 0.0f);
                    any_success = true;
                }
                
                // 4. Pobierz anomalie radaru
                json anomaly_json;
                if (client.getJson("/radar/anomalies", anomaly_json)) {
                    if (anomaly_json.contains("last_anomaly")) {
                        data.last_anomaly_type = anomaly_json["last_anomaly"].value("type", "NONE");
                    }
                }
                
                // 5. Pobierz dane audio
                json audio_json;
                if (client.getJson("/audio/status", audio_json)) {
                    data.audio_rms = audio_json.value("rms_amplitude", 0.0f);
                    data.dominant_frequency = audio_json.value("dominant_frequency", 0.0f);
                    data.bee_activity_index = audio_json.value("bee_activity_index", 0.0f);
                    data.swarm_probability = audio_json.value("swarm_probability", 0.0f);
                    any_success = true;
                }
                
                // 6. Pobierz jakość powietrza
                json air_json;
                if (client.getJson("/airquality/status", air_json)) {
                    data.iaq_index = air_json.value("IAQ_index", 0.0f);
                    data.ventilation_need = air_json.value("ventilation_need", 0);
                    any_success = true;
                }
                
                // Zaktualizuj dane w pamięci współdzielonej
                if (any_success) {
                    std::lock_guard<std::mutex> lock(data_mutex);
                    hives_data[config.hive_id] = data;
                }
                
            } catch (const std::exception& e) {
                error("[" + config.hive_id + "] Błąd podczas pobierania danych: " + std::string(e.what()));
            }
            
            // Oblicz czas wykonania i dostosuj sleep
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            int sleep_time = (config.poll_interval * 1000) - duration.count();
            if (sleep_time > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
            }
        }
        
        info("Zatrzymano zbieranie danych z ula: " + config.hive_id);
    }
    
public:
    ApiaryHttpCollector() {
        // Inicjalizacja CURL globalnie
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~ApiaryHttpCollector() {
        stop();
        curl_global_cleanup();
    }
    
    // Dodaj konfigurację ula
    void addHive(const HiveConfig& config) {
        hive_configs.push_back(config);
        
        // Inicjalizacja struktury danych
        HiveData initial_data;
        initial_data.hive_id = config.hive_id;
        initial_data.is_online = false;
        
        std::lock_guard<std::mutex> lock(data_mutex);
        hives_data[config.hive_id] = initial_data;
        
        info("Dodano ul: " + config.hive_id + " (IP: " + config.pico_ip + 
             ":" + std::to_string(config.pico_port) + ", interval: " + 
             std::to_string(config.poll_interval) + "s)");
    }
    
    // Wczytaj konfigurację z pliku
    bool loadConfigFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            error("Nie można otworzyć pliku konfiguracyjnego: " + filename);
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Pomijaj komentarze i puste linie
            if (line.empty() || line[0] == '#') continue;
            
            // Format: hive_id,ip,port,interval
            std::stringstream ss(line);
            std::string segment;
            std::vector<std::string> parts;
            
            while (std::getline(ss, segment, ',')) {
                parts.push_back(segment);
            }
            
            if (parts.size() >= 4) {
                HiveConfig config;
                config.hive_id = parts[0];
                config.pico_ip = parts[1];
                config.pico_port = std::stoi(parts[2]);
                config.poll_interval = std::stoi(parts[3]);
                addHive(config);
            } else if (parts.size() == 2) {
                // Uproszczony format: hive_id,ip
                HiveConfig config;
                config.hive_id = parts[0];
                config.pico_ip = parts[1];
                config.pico_port = 8080;
                config.poll_interval = 5;
                addHive(config);
            }
        }
        
        file.close();
        info("Wczytano " + std::to_string(hive_configs.size()) + " uli z pliku: " + filename);
        return !hive_configs.empty();
    }
    
    // Start zbierania danych
    void start() {
        if (running) return;
        running = true;
        
        info("Uruchamianie kolektora HTTP z " + std::to_string(hive_configs.size()) + " ulami");
        
        for (const auto& config : hive_configs) {
            worker_threads.emplace_back(&ApiaryHttpCollector::collectHiveData, this, config);
        }
    }
    
    // Stop zbierania danych
    void stop() {
        if (!running) return;
        running = false;
        
        info("Zatrzymywanie kolektora...");
        
        for (auto& t : worker_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        worker_threads.clear();
        
        info("Kolektor zatrzymany.");
    }
    
    // Eksport danych do JSON
    std::string getStatusJSON() {
        std::lock_guard<std::mutex> lock(data_mutex);
        
        json root;
        
        for (const auto& pair : hives_data) {
            const auto& d = pair.second;
            
            json hive_json;
            hive_json["temperature"] = d.temperature;
            hive_json["humidity"] = d.humidity;
            hive_json["weight"] = d.weight;
            hive_json["co2_eq"] = d.co2_eq;
            hive_json["voc_idx"] = d.voc_idx;
            hive_json["hx711_raw"] = d.hx711_raw;
            hive_json["hx711_rate"] = d.hx711_rate;
            hive_json["honey_production_idx"] = d.honey_production_idx;
            hive_json["colony_growth_rate"] = d.colony_growth_rate;
            hive_json["predicted_weight_24h"] = d.predicted_weight_24h;
            hive_json["radar_target_count"] = d.radar_target_count;
            hive_json["radar_distance"] = d.radar_distance;
            hive_json["hive_health_index"] = d.hive_health_index;
            hive_json["activity_ratio"] = d.activity_ratio;
            hive_json["last_anomaly_type"] = d.last_anomaly_type;
            hive_json["audio_rms"] = d.audio_rms;
            hive_json["dominant_frequency"] = d.dominant_frequency;
            hive_json["bee_activity_index"] = d.bee_activity_index;
            hive_json["swarm_probability"] = d.swarm_probability;
            hive_json["iaq_index"] = d.iaq_index;
            hive_json["ventilation_need"] = d.ventilation_need;
            hive_json["is_online"] = d.is_online;
            hive_json["last_update"] = d.last_update;
            hive_json["timestamp"] = d.timestamp;
            hive_json["error_count"] = d.error_count;
            
            root[pair.first] = hive_json;
        }
        
        return root.dump(2);
    }
    
    // Eksport danych do CSV
    std::string getStatusCSV() {
        std::lock_guard<std::mutex> lock(data_mutex);
        
        std::stringstream csv;
        csv << "timestamp,hive_id,status,temperature,humidity,weight,co2_eq,voc_idx,"
            << "hx711_rate,honey_production_idx,colony_growth_rate,predicted_weight_24h,"
            << "radar_targets,radar_distance,hive_health_index,activity_ratio,last_anomaly,"
            << "audio_rms,dominant_freq,bee_activity,swarm_prob,iaq_index,ventilation_need\n";
        
        std::time_t now = std::time(nullptr);
        
        for (const auto& pair : hives_data) {
            const auto& d = pair.second;
            
            std::string status = "OFFLINE";
            if (d.is_online) {
                if (now - d.last_update < 30) {
                    status = "ONLINE";
                } else if (now - d.last_update < 300) {
                    status = "STALE";
                }
            }
            
            csv << d.timestamp << ","
                << d.hive_id << ","
                << status << ","
                << d.temperature << ","
                << d.humidity << ","
                << d.weight << ","
                << d.co2_eq << ","
                << d.voc_idx << ","
                << d.hx711_rate << ","
                << d.honey_production_idx << ","
                << d.colony_growth_rate << ","
                << d.predicted_weight_24h << ","
                << d.radar_target_count << ","
                << d.radar_distance << ","
                << d.hive_health_index << ","
                << d.activity_ratio << ","
                << d.last_anomaly_type << ","
                << d.audio_rms << ","
                << d.dominant_frequency << ","
                << d.bee_activity_index << ","
                << d.swarm_probability << ","
                << d.iaq_index << ","
                << d.ventilation_need << "\n";
        }
        
        return csv.str();
    }
    
    // Wydruk podsumowania
    void printSummary() {
        std::lock_guard<std::mutex> lock(data_mutex);
        
        std::cout << "\n========== PODSUMOWANIE PASIEKI ==========\n" << std::endl;
        
        for (const auto& pair : hives_data) {
            const auto& d = pair.second;
            
            std::string status = d.is_online ? "[ONLINE]" : "[OFFLINE]";
            if (d.is_online && (std::time(nullptr) - d.last_update > 300)) {
                status = "[STALE]";
            }
            
            std::cout << "🐝 " << d.hive_id << " " << status << std::endl;
            std::cout << "   🌡️ Temp: " << d.temperature << "°C  💧 Wilgotność: " << d.humidity << "%" << std::endl;
            std::cout << "   ⚖️ Waga: " << d.weight << "kg  📈 Trend: " << d.hx711_rate << "g/h" << std::endl;
            std::cout << "   🍯 Produkcja miodu: " << (d.honey_production_idx * 100) << "%" << std::endl;
            std::cout << "   📡 Radar: cele=" << d.radar_target_count 
                      << ", zdrowie ula=" << (d.hive_health_index * 100) << "%" << std::endl;
            std::cout << "   🎵 Audio: aktywność pszczół=" << (d.bee_activity_index * 100) 
                      << "%, rojenie=" << (d.swarm_probability * 100) << "%" << std::endl;
            std::cout << "   💨 Jakość powietrza IAQ: " << d.iaq_index << std::endl;
            std::cout << "   🏭 CO₂: " << d.co2_eq << "ppm  VOC: " << d.voc_idx << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "==========================================\n" << std::endl;
    }
};

// Wyświetlenie pomocy
void printHelp(const char* program_name) {
    std::cout << "ApiaryGuard HTTP Collector - Realne zbieranie danych z Raspberry Pi Pico\n\n";
    std::cout << "Użycie:\n";
    std::cout << "  " << program_name << " [opcje]\n\n";
    std::cout << "Opcje:\n";
    std::cout << "  --config <plik>       Wczytaj konfigurację uli z pliku (format: id,ip,[port],[interval])\n";
    std::cout << "  --pico <ip>           Adres IP pojedynczego Pico (domyślnie port 8080)\n";
    std::cout << "  --port <port>         Port HTTP Pico (domyślnie: 8080)\n";
    std::cout << "  --interval <sekundy>  Interwał odpytywania w sekundach (domyślnie: 5)\n";
    std::cout << "  --id <nazwa>          ID ula (domyślnie: UL-1)\n";
    std::cout << "  --output <format>     Format wyjścia: json, csv, summary (domyślnie: summary)\n";
    std::cout << "  --once                Pobierz dane tylko raz i zakończ\n";
    std::cout << "  --help                Wyświetl tę pomoc\n\n";
    std::cout << "Przykłady:\n";
    std::cout << "  " << program_name << " --pico 192.168.1.177 --interval 5\n";
    std::cout << "  " << program_name << " --config hives.conf --output json\n";
    std::cout << "  " << program_name << " --pico 192.168.1.177 --id Ul-Matka --once\n\n";
    std::cout << "Endpointy API Pico (port 8080):\n";
    std::cout << "  /status              - podstawowe dane sensoryczne\n";
    std::cout << "  /hx711/status        - status wagi\n";
    std::cout << "  /hx711/metrics       - metryki wagi (30+ parametrów)\n";
    std::cout << "  /radar/status        - status radaru MMWave\n";
    std::cout << "  /radar/anomalies     - anomalie radaru\n";
    std::cout << "  /audio/status        - status analizy audio\n";
    std::cout << "  /audio/spectrum      - widmo FFT audio\n";
    std::cout << "  /airquality/status   - jakość powietrza w ulu\n";
    std::cout << "  /airquality/metrics  - metryki jakości powietrza\n";
}

int main(int argc, char* argv[]) {
    std::cout << "🐝 ApiaryGuard HTTP Collector v1.0\n";
    std::cout << "Realne zbieranie danych z Raspberry Pi Pico przez Ethernet\n\n";
    
    ApiaryHttpCollector collector;
    
    std::string config_file;
    std::string single_pico_ip;
    int single_pico_port = 8080;
    int poll_interval = 5;
    std::string hive_id = "UL-1";
    std::string output_format = "summary";
    bool run_once = false;
    
    // Parsowanie argumentów
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        }
        else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        }
        else if (arg == "--pico" && i + 1 < argc) {
            single_pico_ip = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc) {
            single_pico_port = std::stoi(argv[++i]);
        }
        else if (arg == "--interval" && i + 1 < argc) {
            poll_interval = std::stoi(argv[++i]);
        }
        else if (arg == "--id" && i + 1 < argc) {
            hive_id = argv[++i];
        }
        else if (arg == "--output" && i + 1 < argc) {
            output_format = argv[++i];
        }
        else if (arg == "--once") {
            run_once = true;
        }
        else {
            std::cerr << "Nieznana opcja: " << arg << std::endl;
            printHelp(argv[0]);
            return 1;
        }
    }
    
    // Wczytaj konfigurację
    if (!config_file.empty()) {
        if (!collector.loadConfigFromFile(config_file)) {
            std::cerr << "Błąd wczytywania konfiguracji z pliku: " << config_file << std::endl;
            return 1;
        }
    }
    else if (!single_pico_ip.empty()) {
        HiveConfig config;
        config.hive_id = hive_id;
        config.pico_ip = single_pico_ip;
        config.pico_port = single_pico_port;
        config.poll_interval = poll_interval;
        collector.addHive(config);
    }
    else {
        std::cerr << "Błąd: Podaj --config <plik> lub --pico <ip>\n";
        std::cerr << "Użyj --help aby wyświetlić pomoc.\n";
        return 1;
    }
    
    // Start zbierania
    collector.start();
    
    // Poczekaj na pierwsze dane
    std::this_thread::sleep_for(std::chrono::seconds(poll_interval + 2));
    
    if (run_once) {
        // Tryb jednorazowy
        if (output_format == "json") {
            std::cout << collector.getStatusJSON() << std::endl;
        }
        else if (output_format == "csv") {
            std::cout << collector.getStatusCSV() << std::endl;
        }
        else {
            collector.printSummary();
        }
        
        collector.stop();
        return 0;
    }
    
    // Tryb ciągły
    std::cout << "\nTryb ciągły. Naciśnij Ctrl+C aby zatrzymać.\n";
    std::cout << "Dane będą odświeżane co " << poll_interval << " sekund.\n\n";
    
    int iteration = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(poll_interval));
        
        iteration++;
        std::time_t now = std::time(nullptr);
        char time_buf[64];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        
        std::cout << "\n--- Aktualizacja #" << iteration << " [" << time_buf << "] ---\n";
        
        if (output_format == "json") {
            std::cout << collector.getStatusJSON() << std::endl;
        }
        else if (output_format == "csv") {
            std::cout << collector.getStatusCSV() << std::endl;
        }
        else {
            collector.printSummary();
        }
    }
    
    collector.stop();
    return 0;
}
