## 📁 Struktura Katalogów i Plików

```
/workspace/
├── README.md                           # Dokumentacja główna
├── LICENSE                             # Licencja projektu (MIT/Apache 2.0)
├── docs/                               # Dodatkowa dokumentacja
│   ├── architecture/                   # Diagramy i specyfikacje architektoniczne
│   │   ├── system_architecture.md
│   │   ├── electrical_schematics.pdf
│   │   ├── mechanical_drawings.dxf
│   │   └── network_topology.md
│   ├── api/                            # Dokumentacja API
│   │   ├── rest_api_spec.yaml
│   │   ├── http_endpoints.md
│   │   └── webhook_examples.md
│   ├── manuals/                        # Podręczniki użytkownika
│   │   ├── installation_guide.md
│   │   ├── calibration_procedures.md
│   │   ├── maintenance_manual.md
│   │   └── troubleshooting_guide.md
│   └── research/                       # Materiały badawcze
│       ├── bee_acoustics_analysis.md
│       ├── thermal_therapy_studies.md
│       └── sensor_fusion_algorithms.md
│
├── hardware/                           # Projekty sprzętowe
│   ├── pico/                           # Firmware Raspberry Pi Pico (C++)
│   │   ├── src/
│   │   │   ├── main.cpp                # Główna pętla Pico
│   │   │   ├── sensors/
│   │   │   │   ├── hx711_driver.cpp    # Sterowanie wagą
│   │   │   │   ├── microphone_adc.cpp  # Akwizycja audio
│   │   │   │   ├── dht22_reader.cpp    # Temp/wilgotność
│   │   │   │   ├── piezo_handler.cpp   # Wibracje
│   │   │   │   └── sensor_fusion.cpp   # Fuzja danych sensorycznych
│   │   │   ├── actuators/
│   │   │   │   ├── heater_control.cpp  # PID grzałki
│   │   │   │   ├── fan_pwm.cpp         # Sterowanie wentylatorem
│   │   │   │   ├── dispenser_pump.cpp  # Dozowanie leków
│   │   │   │   └── valve_control.cpp   # Zawory elektromagnetyczne
│   │   │   ├── communication/
│   │   │   │   ├── http_client.cpp     # HTTP klient dla komunikacji z RPi2
│   │   │   │   ├── uart_protocol.cpp   # Protokół szeregowy (UART)
│   │   │   │   └── message_queue.cpp   # Kolejka wiadomości
│   │   │   ├── utils/
│   │   │   │   ├── watchdog.cpp        # Watchdog timer
│   │   │   │   ├── eeprom_storage.cpp  # Persistent storage
│   │   │   │   └── calibration.cpp     # Procedury kalibracji
│   │   │   └── config/
│   │   │       ├── pin_definitions.h   # Mapowanie pinów
│   │   │       ├── constants.h         # Stałe systemowe
│   │   │       └── thresholds.h        # Progi alarmowe
│   │   ├── lib/                        # Biblioteki Pico
│   │   │   ├── HX711/
│   │   │   ├── DHT-sensor-library/
│   │   │   └── PID-AutoTune/
│   │   ├── CMakeLists.txt              # Konfiguracja CMake dla Pico SDK
│   │   └── Makefile                    # Alternatywny build system
│   │
│   ├── raspberry_pi/                   # Oprogramowanie Raspberry Pi 2
│   │   ├── src/
│   │   │   ├── cpp/                    # Główne aplikacje C++ (TUI/GUI)
│   │   │   │   ├── apiary_tui/
│   │   │   │   │   ├── apiary_tui.cpp      # Terminal User Interface
│   │   │   │   │   ├── models/
│   │   │   │   │   │   ├── Hive.hpp          # Model ula
│   │   │   │   │   │   ├── Apiary.hpp        # Model pasieki
│   │   │   │   │   │   ├── Swarm.hpp         # Model roju
│   │   │   │   │   │   ├── SensorData.hpp    # Dane sensoryczne
│   │   │   │   │   │   ├── Alert.hpp         # Alerty i powiadomienia
│   │   │   │   │   │   └── Treatment.hpp     # Zabiegi terapeutyczne
│   │   │   │   │   ├── services/
│   │   │   │   │   │   ├── IHttpService.hpp    # HTTP klient/serwer
│   │   │   │   │   │   ├── ISensorService.hpp  # Interfejs sensorów
│   │   │   │   │   │   ├── IActuatorService.hpp# Interfejs efektorów
│   │   │   │   │   │   ├── IAnalyticsService.hpp# Analityka
│   │   │   │   │   │   └── INotificationService.hpp # Powiadomienia
│   │   │   │   │   ├── repositories/
│   │   │   │   │   │   ├── SqliteRepository.cpp  # SQLite implementacja
│   │   │   │   │   │   └── CacheRepository.cpp   # Local cache
│   │   │   │   │   └── helpers/
│   │   │   │   │       ├── DateTimeUtils.cpp
│   │   │   │   │       ├── JsonSerializers.cpp
│   │   │   │   │       └── UnitConverters.cpp
│   │   │   │   ├── apiary_collector/
│   │   │   │   │   ├── apiary_collector.cpp    # Kolektor danych z Pico (HTTP)
│   │   │   │   │   └── background_services/
│   │   │   │   │       ├── DataAcquisition.cpp # Pobieranie danych
│   │   │   │   │       ├── Analytics.cpp       # Analiza w tle
│   │   │   │   │       ├── LteMonitor.cpp      # Monitor łącza
│   │   │   │   │       └── MaintenanceScheduler.cpp # Harmonogram konserwacji
│   │   │   │   └── workers/
│   │   │   │       ├── BeeSoundAnalyzer.cpp    # Analiza audio
│   │   │   │       ├── WeightTrend.cpp         # Trendy wagowe
│   │   │   │       ├── DiseasePrediction.cpp   # Predykcja chorób
│   │   │   │       └── SwarmPrediction.cpp     # Predykcja rojenia
│   │   │   ├── bash/                   # Skrypty Bash
│   │   │   │   ├── system/
│   │   │   │   │   ├── install.sh          # Instalacja systemu
│   │   │   │   │   ├── update.sh           # Aktualizacja oprogramowania
│   │   │   │   │   ├── backup.sh           # Backup danych i konfiguracji
│   │   │   │   │   ├── restore.sh          # Przywracanie z backupu
│   │   │   │   │   ├── health_check.sh     # Sprawdzenie zdrowia systemu
│   │   │   │   │   ├── log_rotation.sh     # Rotacja logów
│   │   │   │   │   ├── security_hardening.sh # Hardening bezpieczeństwa
│   │   │   │   │   └── factory_reset.sh    # Przywrócenie ustawień fabrycznych
│   │   │   │   ├── network/
│   │   │   │   │   ├── wifi_setup.sh       # Konfiguracja WiFi
│   │   │   │   │   ├── network_monitor.sh  # Monitorowanie połączenia
│   │   │   │   │   ├── firewall_setup.sh   # Konfiguracja iptables
│   │   │   │   │   └── bandwidth_test.sh   # Test przepustowości
│   │   │   │   ├── sensors/
│   │   │   │   │   ├── calibrate_scale.sh  # Kalibracja wagi
│   │   │   │   │   ├── test_microphone.sh  # Test mikrofonu
│   │   │   │   │   ├── read_all_sensors.sh # Odczyt wszystkich sensorów
│   │   │   │   │   └── sensor_diagnostics.sh # Diagnostyka sensorów
│   │   │   │   ├── services/
│   │   │   │   │   ├── apache_install.sh   # Instalacja Apache2
│   │   │   │   │   ├── database_init.sh    # Inicjalizacja bazy danych
│   │   │   │   │   ├── start_all.sh        # Start wszystkich usług
│   │   │   │   │   ├── stop_all.sh         # Stop wszystkich usług
│   │   │   │   │   └── restart_failed.sh   # Restart failed services
│   │   │   │   └── utilities/
│   │   │   │       ├── disk_cleanup.sh     # Czyszczenie dysku
│   │   │   │       ├── memory_monitor.sh   # Monitor pamięci
│   │   │   │       ├── temperature_log.sh  # Logowanie temperatur CPU
│   │   │   │       ├── uptime_report.sh    # Raport uptime
│   │   │   │       └── generate_cert.sh    # Generowanie certyfikatów SSL
│   │   │   └── signal_processing/
│   │   │       ├── CMakeLists.txt
│   │   │       ├── include/
│   │   │       │   ├── fft_analyzer.hpp      # FFT analiza audio
│   │   │       │   ├── digital_filter.hpp    # Filtry cyfrowe
│   │   │       │   ├── spectrogram.hpp       # Spektrogramy
│   │   │       │   └── feature_extractor.hpp # Ekstrakcja cech
│   │   │       └── src/
│   │   │           ├── fft_analyzer.cpp
│   │   │           ├── digital_filter.cpp
│   │   │           ├── spectrogram.cpp
│   │   │           └── feature_extractor.cpp
│   │   ├── config/
│   │   │   ├── apache2/
│   │   │   │   ├── 000-default.conf    # Apache virtual host config
│   │   │   │   ├── ssl.conf            # SSL/TLS configuration
│   │   │   │   └── htpasswd            # Basic auth passwords
│   │   │   ├── systemd/
│   │   │   │   ├── apiaryguard-tui.service
│   │   │   │   ├── apiaryguard-collector.service
│   │   │   │   └── network-watchdog.service
│   │   │   ├── network/
│   │   │   │   ├── interfaces          # Network interfaces config
│   │   │   │   └── wpa_supplicant.conf # WiFi config
│   │   │   └── database/
│   │   │       ├── schema.sql
│   │   │       ├── indexes.sql
│   │   │       └── seed_data.sql
│   │   ├── tests/
│   │   │   ├── unit/
│   │   │   │   ├── SensorTests.cpp
│   │   │   │   ├── ActuatorTests.cpp
│   │   │   │   └── ModelTests.cpp
│   │   │   └── integration/
│   │   │       └── HttpIntegrationTests.cpp
│   │   └── Dockerfile                  # Containerization (opcjonalne)
│   │
│   ├── mechanical/                     # Projekty mechaniczne
│   │   ├── enclosure/
│   │   │   ├── main_housing.step       # CAD 3D model
│   │   │   ├── main_housing.stl        # 3D print file
│   │   │   ├── mounting_bracket.dxf    # Laser cutting file
│   │   │   └── assembly_instructions.md
│   │   ├── sensor_mounts/
│   │   │   ├── weight_platform.step
│   │   │   ├── microphone_holder.stl
│   │   │   └── temp_probe_guard.step
│   │   ├── actuator_housings/
│   │   │   ├── pump_mount.step
│   │   │   ├── heater_shield.step
│   │   │   └── fan_duct.step
│   │   └── bom/                        # Bill of Materials
│   │       ├── electronics_bom.csv
│   │       ├── mechanical_bom.csv
│   │       └── assembly_bom.csv
│   │
│   └── schematics/                     # Schematy elektryczne
│       ├── power_distribution.pdf
│       ├── sensor_wiring.pdf
│       ├── actuator_control.pdf
│       └── communication_bus.pdf
│
├── src/                                # Kod źródłowy (skrócona struktura)
│   ├── pico/                           # Firmware dla Raspberry Pi Pico
│   │   ├── apiaryguard_pico.ino
│   │   └── README.md
│   ├── pico_w6100/                     # Firmware Pico + Ethernet W6100
│   │   ├── apiaryguard_pico_w6100.ino
│   │   └── README.md
│   ├── rpi_tui/                        # TUI dla Raspberry Pi 2
│   │   ├── apiary_tui.sh
│   │   ├── apiary_logger.cpp
│   │   ├── apiary_collector.cpp
│   │   ├── Makefile
│   │   └── README.md
│   └── arduino/                        # [ARCHIWUM] Stary kod Arduino Nano
│       └── README.md                   # Tylko referencyjnie
│
└── tools/                              # Narzędzia pomocnicze
    ├── flash_pico.sh
    ├── backup_config.sh
    └── diagnostic_tools.sh
```
