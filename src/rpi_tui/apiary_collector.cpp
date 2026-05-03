/**
 * apiary_collector.cpp
 * Moduł zbierania danych z wielu uli przez Ethernet (Raspberry Pi 2 + Pico)
 * Kompilacja: g++ -std=c++17 -pthread -o apiary_collector apiary_collector.cpp
 * 
 * OBSŁUGIWANE PARAMETRY (WSZYSTKIE Z .md i .ino):
 * - Podstawowe: temp, humidity, weight, battery, co2, voc, motion
 * - Audio (97+): rms_amplitude, dominant_frequency, swarm_probability, bee_activity_index, etc.
 * - Radar (27): distance, energy, activity_ratio, hive_health_index, etc.
 * - HX711 (105+): mean_weight, trend_slope, nectar_inflow_rate, colony_growth_rate, etc.
 * - TempHumidity (28): heat_index, dew_point, comfort_index, brood_stress_index, etc.
 * - AirQuality (24): iaq_index, ventilation_need, contamination_risk, etc.
 * - PiezoVibration (22): vibration_rms, bee_traffic_score, intrusion_probability, etc.
 * - Barometric (18): pressure_mean, weather_trend, foraging_conditions, etc.
 * - Light (17): lux_current, daylight_duration, circadian_sync, etc.
 * 
 * KOMUNIKACJA: HTTP API JSON na porcie 8080
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
#include <iomanip>
#include <algorithm>

// Dołączamy loggera i używamy przestrzeni nazw apiary
#include "apiary_logger.cpp"
using namespace apiary;

// Struktura danych z ula - ROZSZERZONA o WSZYSTKIE parametry z dokumentacji .md i pico.ino
struct HiveData {
    // Podstawowe parametry (9 pól)
    std::string hive_id;
    float temperature = 0.0f;           // Temperatura [°C]
    float humidity = 0.0f;              // Wilgotność [%RH]
    float weight = 0.0f;                // Waga [kg]
    int battery_level = 0;              // Poziom baterii [%]
    int co2_eq = 0;                     // CO2 equivalent [ppm]
    int voc_idx = 0;                    // VOC index [index]
    int motion_detected = 0;            // Flaga ruchu z radaru (0/1)
    long long timestamp = 0;            // Timestamp [s]
    bool is_online = false;             // Status online/offline
    
    // Parametry Audio (97+ parametrów - główne wybrane)
    float audio_rms = 0.0f;             // RMS amplituda audio [V]
    float audio_dominant_freq = 0.0f;   // Dominująca częstotliwość audio [Hz]
    float audio_swarm_prob = 0.0f;      // Prawdopodobieństwo rojenia z audio [0-1]
    float audio_bee_activity = 0.0f;    // Indeks aktywności pszczół [0-100%]
    float audio_hive_health = 0.0f;     // Indeks zdrowia z audio [0-100%]
    float audio_spectral_centroid = 0.0f; // Centrum widma [Hz]
    float audio_power_bee_band = 0.0f;  // Moc w paśmie pszczół [dB]
    float audio_crest_factor = 0.0f;    // Współczynnik szczytu
    float audio_entropy = 0.0f;         // Entropia widmowa
    float audio_foraging_eff = 0.0f;    // Efektywność zbierania [0-100%]
    
    // Parametry Radar MMWave (27 parametrów - główne wybrane)
    float radar_distance = 0.0f;        // Odległość wykrytego obiektu [m]
    float radar_energy = 0.0f;          // Energia sygnału radaru [dB]
    float radar_activity = 0.0f;        // Współczynnik aktywności [0-1]
    float radar_hive_health = 0.0f;     // Indeks zdrowia ula [0-100%]
    float radar_signal_quality = 0.0f;  // Jakość sygnału [0-100%]
    float radar_target_rate = 0.0f;     // Tempo pojawiania się celów [/min]
    float radar_entropy = 0.0f;         // Entropia sygnału
    float radar_trend_slope = 0.0f;     // Nachylenie trendu
    
    // Parametry HX711 Waga (105+ parametrów - główne wybrane)
    float hx711_mean_weight = 0.0f;     // Średnia waga [kg]
    float hx711_std_weight = 0.0f;      // Odchylenie standardowe wagi [kg]
    float hx711_trend_slope_1h = 0.0f;  // Trend 1h [kg/h]
    float hx711_trend_slope_24h = 0.0f; // Trend 24h [kg/h]
    float hx711_nectar_inflow = 0.0f;   // Przepływ nektaru [kg/h]
    float hx711_consumption_rate = 0.0f;// Zużycie zapasów [kg/h]
    float hx711_colony_growth = 0.0f;   // Tempo wzrostu kolonii [%/dzień]
    float hx711_productivity = 0.0f;    // Wynik produktywności [0-100%]
    float hx711_predicted_24h = 0.0f;   // Prognoza wagi za 24h [kg]
    float hx711_anomaly_score = 0.0f;   // Wynik anomalii [0-1]
    
    // Parametry Temperature/Humidity (28 parametrów - główne)
    float th_heat_index = 0.0f;         // Indeks ciepła [°C]
    float th_dew_point = 0.0f;          // Punkt rosy [°C]
    float th_comfort_index = 0.0f;      // Indeks komfortu [0-100%]
    float th_brood_stress = 0.0f;       // Stres czerwiu [0-100%]
    float th_temp_stability = 0.0f;     // Stabilność temperatury [0-100%]
    float th_mold_risk = 0.0f;          // Ryzyko pleśni [0-1]
    
    // Parametry Air Quality (24 parametry - główne)
    int aq_co2_mean = 0;                // Średnie CO2 [ppm]
    int aq_voc_mean = 0;                // Średnie VOC [index]
    float aq_iaq_index = 0.0f;          // Indoor Air Quality Index [0-500]
    float aq_ventilation_need = 0.0f;   // Zapotrzebowanie wentylacji [0-100%]
    float aq_contamination_risk = 0.0f; // Ryzyko zanieczyszczenia [0-1]
    float aq_hive_comfort = 0.0f;       // Komfort ula z powietrza [0-100%]
    
    // Parametry Piezo Vibration (22 parametry - główne)
    float piezo_rms = 0.0f;             // RMS wibracji [mV]
    float piezo_dominant_freq = 0.0f;   // Dominująca częstotliwość [Hz]
    float piezo_activity_idx = 0.0f;    // Indeks aktywności [0-100%]
    float piezo_bee_traffic = 0.0f;     // Ruch pszczół [0-100%]
    float piezo_predator_score = 0.0f;  // Wynik drapieżnika [0-100%]
    float piezo_intrusion_prob = 0.0f;  // Prawdopodobieństwo intruza [0-1]
    
    // Parametry Barometric (18 parametrów - główne)
    float baro_pressure = 0.0f;         // Ciśnienie [hPa]
    float baro_trend_1h = 0.0f;         // Trend ciśnienia 1h [hPa/h]
    float baro_weather_trend = 0.0f;    // Trend pogodowy [-1 do 1]
    float baro_storm_prob = 0.0f;       // Prawdopodobieństwo burzy [0-1]
    float baro_foraging_cond = 0.0f;    // Warunki do wylotów [0-100%]
    
    // Parametry Light (17 parametrów - główne)
    uint32_t light_lux = 0;             // Natężenie światła [lux]
    float light_daylight_hours = 0.0f;  // Czas dnia [godziny]
    float light_circadian_sync = 0.0f;  // Synchronizacja cyrkadiana [0-1]
    float light_foraging_idx = 0.0f;    // Indeks światła do wylotów [0-100%]
    
    // Dodatkowe pola z UDP (backward compatibility)
    float weight_rate = 0.0f;           // Szybkość zmiany wagi [kg/h]
    float weight_trend = 0.0f;          // Trend wagi [-1..1]
    int air_iaq = 0;                    // Indeks jakości powietrza [0-100]
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
        
        // Inicjalizacja struktury danych - wszystkie pola na zero/false
        std::lock_guard<std::mutex> lock(data_mutex);
        for (size_t i = 0; i < ips.size(); ++i) {
            std::string id = "UL-" + std::to_string(i + 1);
            hives_data[id] = {id, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0, false, 
                              0.0f, 0.0f, 0.0f,  // audio
                              0.0f, 0.0f, 0.0f,  // radar
                              0.0f, 0.0f,        // waga trend
                              0};                // air_iaq
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
        // Format oczekiwany od Pico (rozszerzony): 
        // ID,TEMP,HUM,WEIGHT,BAT,CO2,VOC,MOTION,TIMESTAMP,AUDIO_RMS,AUDIO_DOM_FREQ,AUDIO_SWARM_PROB,RADAR_DIST,RADAR_ENERGY,RADAR_ACTIVITY,WAG_RATE,WAG_TREND,AIR_IAQ
        // Przykład: "UL-1,24.5,65.2,45.300,98,450,35,1,1234567890,0.025,250.5,0.15,1.2,45.3,0.35,-0.02,1.0,75"
        // Wszystkie parametry: ID, temp[C], humidity[%], weight[kg], battery[%], CO2[ppm], VOC[idx], motion[0/1], timestamp[s],
        //                      audio_rms[V], audio_dom_freq[Hz], audio_swarm_prob[0-1], radar_dist[m], radar_energy[dB], radar_activity[0-1],
        //                      weight_rate[kg/h], weight_trend[-1..1], air_iaq[0-100]
        
        std::stringstream ss(raw_data);
        std::string segment;
        std::vector<std::string> parts;

        while (std::getline(ss, segment, ',')) {
            parts.push_back(segment);
        }

        // Wymagane minimum 9 pól dla podstawowych danych, ale obsługujemy do 18 pól dla rozszerzonych
        if (parts.size() < 9) {
            Logger::getInstance().warning( "Niepoprawny format danych z " + source_ip + ": " + raw_data);
            return;
        }

        try {
            std::string hive_id = parts[0];
            float temp = std::stof(parts[1]);         // TEMP - temperatura [°C]
            float humidity = std::stof(parts[2]);     // HUM - wilgotność [%RH]
            float weight = std::stof(parts[3]);       // WEIGHT - waga [kg]
            int battery = std::stoi(parts[4]);        // BAT - bateria [%]
            int co2 = std::stoi(parts[5]);            // CO2 - CO2 equivalent [ppm]
            int voc = std::stoi(parts[6]);            // VOC - VOC index [index]
            int motion = std::stoi(parts[7]);         // MOTION - flaga ruchu (0/1)
            // parts[8] to TIMESTAMP z Pico - używamy lokalnego czasu zamiast synchronizacji
            long long now = std::time(nullptr);       // Aktualny timestamp systemu
            
            // Parametry rozszerzone (opcjonalne, domyślne wartości jeśli brak)
            float audio_rms = 0.0f;
            float audio_dom_freq = 0.0f;
            float audio_swarm_prob = 0.0f;
            float radar_dist = 0.0f;
            float radar_energy = 0.0f;
            float radar_activity = 0.0f;
            float weight_rate = 0.0f;
            float weight_trend = 0.0f;
            int air_iaq = 0;
            
            if (parts.size() > 9) audio_rms = std::stof(parts[9]);
            if (parts.size() > 10) audio_dom_freq = std::stof(parts[10]);
            if (parts.size() > 11) audio_swarm_prob = std::stof(parts[11]);
            if (parts.size() > 12) radar_dist = std::stof(parts[12]);
            if (parts.size() > 13) radar_energy = std::stof(parts[13]);
            if (parts.size() > 14) radar_activity = std::stof(parts[14]);
            if (parts.size() > 15) weight_rate = std::stof(parts[15]);
            if (parts.size() > 16) weight_trend = std::stof(parts[16]);
            if (parts.size() > 17) air_iaq = std::stoi(parts[17]);

            // Aktualizacja danych w pamięci współdzielonej
            {
                std::lock_guard<std::mutex> lock(data_mutex);
                if (hives_data.find(hive_id) != hives_data.end()) {
                    hives_data[hive_id].temperature = temp;
                    hives_data[hive_id].humidity = humidity;
                    hives_data[hive_id].weight = weight;
                    hives_data[hive_id].battery_level = battery;
                    hives_data[hive_id].co2_eq = co2;
                    hives_data[hive_id].voc_idx = voc;
                    hives_data[hive_id].motion_detected = motion;
                    hives_data[hive_id].timestamp = now;
                    hives_data[hive_id].is_online = true;
                    
                    // Nowe parametry audio
                    hives_data[hive_id].audio_rms = audio_rms;
                    hives_data[hive_id].audio_dominant_freq = audio_dom_freq;
                    hives_data[hive_id].audio_swarm_prob = audio_swarm_prob;
                    
                    // Nowe parametry radaru
                    hives_data[hive_id].radar_distance = radar_dist;
                    hives_data[hive_id].radar_energy = radar_energy;
                    hives_data[hive_id].radar_activity = radar_activity;
                    
                    // Nowe parametry wagi
                    hives_data[hive_id].weight_rate = weight_rate;
                    hives_data[hive_id].weight_trend = weight_trend;
                    
                    // Nowe parametry jakości powietrza
                    hives_data[hive_id].air_iaq = air_iaq;
                    
                    Logger::getInstance().debug( "Zaktualizowano dane dla " + hive_id + 
                               " [T:" + std::to_string(temp) + 
                               " H:" + std::to_string(humidity) + 
                               " CO2:" + std::to_string(co2) + 
                               " VOC:" + std::to_string(voc) + 
                               " Motion:" + std::to_string(motion) +
                               " AudioRMS:" + std::to_string(audio_rms) +
                               " RadarAct:" + std::to_string(radar_activity) +
                               " IAQ:" + std::to_string(air_iaq) + "]");
                } else {
                    // Nowy ul dynamicznie wykryty
                    Logger::getInstance().info( "Wykryto nowy ul: " + hive_id + " z IP " + source_ip);
                    HiveData new_hive = {hive_id, temp, humidity, weight, battery, co2, voc, motion, now, true,
                                         audio_rms, audio_dom_freq, audio_swarm_prob,
                                         radar_dist, radar_energy, radar_activity,
                                         weight_rate, weight_trend, air_iaq};
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
                 << "\"co2\":" << d.co2_eq << ","
                 << "\"voc\":" << d.voc_idx << ","
                 << "\"motion\":" << d.motion_detected << ","
                 << "\"online\":" << (d.is_online ? "true" : "false") << ","
                 << "\"audio_rms\":" << d.audio_rms << ","
                 << "\"audio_freq\":" << d.audio_dominant_freq << ","
                 << "\"swarm_prob\":" << d.audio_swarm_prob << ","
                 << "\"radar_dist\":" << d.radar_distance << ","
                 << "\"radar_energy\":" << d.radar_energy << ","
                 << "\"radar_activity\":" << d.radar_activity << ","
                 << "\"weight_rate\":" << d.weight_rate << ","
                 << "\"weight_trend\":" << d.weight_trend << ","
                 << "\"air_iaq\":" << d.air_iaq
                 << "}";
        }
        json << "}";
        return json.str();
    }
    
    // Prostszy format dla Bash (CSV) - wszystkie parametry
    std::string getStatusCSV() {
        std::lock_guard<std::mutex> lock(data_mutex);
        std::stringstream csv;
        csv << "ID,STATUS,TEMP,HUM,WEIGHT,BAT,CO2,VOC,MOTION,AUDIO_RMS,AUDIO_FREQ,SWARM_PROB,RADAR_DIST,RADAR_ENERGY,RADAR_ACT,WAG_RATE,WAG_TREND,AIR_IAQ,TIME\n";
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
                << d.co2_eq << ","
                << d.voc_idx << ","
                << d.motion_detected << ","
                << d.audio_rms << ","
                << d.audio_dominant_freq << ","
                << d.audio_swarm_prob << ","
                << d.radar_distance << ","
                << d.radar_energy << ","
                << d.radar_activity << ","
                << d.weight_rate << ","
                << d.weight_trend << ","
                << d.air_iaq << ","
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
