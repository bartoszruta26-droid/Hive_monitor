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
│   │   ├── mqtt_topics.md
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
│   ├── arduino_nano/                   # Firmware Arduino Nano
│   │   ├── src/
│   │   │   ├── main.cpp                # Główna pętla Arduino
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
│   │   │   │   ├── i2c_slave.cpp       # I2C komunikacja z RPi
│   │   │   │   ├── uart_protocol.cpp   # Protokół szeregowy
│   │   │   │   └── message_queue.cpp   # Kolejka wiadomości
│   │   │   ├── utils/
│   │   │   │   ├── watchdog.cpp        # Watchdog timer
│   │   │   │   ├── eeprom_storage.cpp  # Persistent storage
│   │   │   │   └── calibration.cpp     # Procedury kalibracji
│   │   │   └── config/
│   │   │       ├── pin_definitions.h   # Mapowanie pinów
│   │   │       ├── constants.h         # Stałe systemowe
│   │   │       └── thresholds.h        # Progi alarmowe
│   │   ├── lib/                        # Biblioteki Arduino
│   │   │   ├── HX711/
│   │   │   ├── DHT-sensor-library/
│   │   │   └── PID-AutoTune/
│   │   ├── platformio.ini              # Konfiguracja PlatformIO
│   │   └── Makefile                    # Alternatywny build system
│   │
│   ├── raspberry_pi/                   # Oprogramowanie Raspberry Pi
│   │   ├── src/
│   │   │   ├── CSharp/                 # Główne aplikacje C# (.NET Core)
│   │   │   │   ├── ApiaryGuard.Core/
│   │   │   │   │   ├── ApiaryGuard.Core.csproj
│   │   │   │   │   ├── Models/
│   │   │   │   │   │   ├── Hive.cs           # Model ula
│   │   │   │   │   │   ├── Apiary.cs         # Model pasieki
│   │   │   │   │   │   ├── Swarm.cs          # Model rójki
│   │   │   │   │   │   ├── SensorData.cs     # Dane sensoryczne
│   │   │   │   │   │   ├── Alert.cs          # Alerty i powiadomienia
│   │   │   │   │   │   └── Treatment.cs      # Zabiegi terapeutyczne
│   │   │   │   │   ├── Services/
│   │   │   │   │   │   ├── IDataRepository.cs    # Interfejs repozytorium
│   │   │   │   │   │   ├── ISensorService.cs   # Interfejs sensorów
│   │   │   │   │   │   ├── IActuatorService.cs # Interfejs efektorów
│   │   │   │   │   │   ├── IMqttService.cs     # MQTT broker client
│   │   │   │   │   │   ├── ILteService.cs      # Obsługa LTE
│   │   │   │   │   │   ├── IAnalyticsService.cs# Analityka
│   │   │   │   │   │   └── INotificationService.cs # Powiadomienia
│   │   │   │   │   ├── Repositories/
│   │   │   │   │   │   ├── SqliteRepository.cs   # SQLite implementacja
│   │   │   │   │   │   ├── InfluxRepository.cs   # InfluxDB time-series
│   │   │   │   │   │   └── CacheRepository.cs    # Redis cache
│   │   │   │   │   ├── Controllers/
│   │   │   │   │   │   ├── SensorController.cs   # API endpoints sensory
│   │   │   │   │   │   ├── ActuatorController.cs # API endpoints aktuary
│   │   │   │   │   │   ├── HiveController.cs     # CRUD operacje na ulach
│   │   │   │   │   │   ├── AlertController.cs    # Zarządzanie alertami
│   │   │   │   │   │   └── ReportController.cs   # Generowanie raportów
│   │   │   │   │   ├── Middleware/
│   │   │   │   │   │   ├── ExceptionHandler.cs   # Global error handling
│   │   │   │   │   │   ├── Authentication.cs     # JWT authentication
│   │   │   │   │   │   └── RateLimiter.cs        # API rate limiting
│   │   │   │   │   └── Helpers/
│   │   │   │   │       ├── DateTimeExtensions.cs
│   │   │   │   │       ├── JsonSerializers.cs
│   │   │   │   │       └── UnitConverters.cs
│   │   │   │   ├── ApiaryGuard.Worker/
│   │   │   │   │   ├── ApiaryGuard.Worker.csproj
│   │   │   │   │   ├── BackgroundServices/
│   │   │   │   │   │   ├── DataAcquisitionHostedService.cs # Pobieranie danych
│   │   │   │   │   │   ├── AnalyticsBackgroundService.cs   # Analiza w tle
│   │   │   │   │   │   ├── MqttPublisherService.cs         # Publikacja MQTT
│   │   │   │   │   │   ├── LteMonitorService.cs            # Monitor łącza
│   │   │   │   │   │   └── MaintenanceSchedulerService.cs  # Harmonogram konserwacji
│   │   │   │   │   └── Workers/
│   │   │   │   │       ├── BeeSoundAnalyzerWorker.cs       # Analiza audio
│   │   │   │   │       ├── WeightTrendWorker.cs            # Trendy wagowe
│   │   │   │   │       ├── DiseasePredictionWorker.cs      # Predykcja chorób
│   │   │   │   │       └── SwarmPredictionWorker.cs        # Predykcja rojenia
│   │   │   │   ├── ApiaryGuard.WebApi/
│   │   │   │   │   ├── ApiaryGuard.WebApi.csproj
│   │   │   │   │   ├── Program.cs
│   │   │   │   │   ├── Startup.cs
│   │   │   │   │   ├── appsettings.json
│   │   │   │   │   └── Controllers/
│   │   │   │   └── ApiaryGuard.CLI/
│   │   │   │       ├── ApiaryGuard.CLI.csproj
│   │   │   │       └── Commands/
│   │   │   │           ├── CalibrateCommand.cs
│   │   │   │           ├── DiagnosticCommand.cs
│   │   │   │           ├── BackupCommand.cs
│   │   │   │           └── UpdateCommand.cs
│   │   │   │
│   │   │   ├── CPP/                    # Wysokowydajne moduły C++
│   │   │   │   ├── signal_processing/
│   │   │   │   │   ├── CMakeLists.txt
│   │   │   │   │   ├── include/
│   │   │   │   │   │   ├── fft_analyzer.hpp      # FFT analiza audio
│   │   │   │   │   │   ├── digital_filter.hpp    # Filtry cyfrowe
│   │   │   │   │   │   ├── spectrogram.hpp       # Spektrogramy
│   │   │   │   │   │   └── feature_extractor.hpp # Ekstrakcja cech
│   │   │   │   │   └── src/
│   │   │   │   │       ├── fft_analyzer.cpp
│   │   │   │   │       ├── digital_filter.cpp
│   │   │   │   │       ├── spectrogram.cpp
│   │   │   │   │       └── feature_extractor.cpp
│   │   │   │   ├── machine_learning/
│   │   │   │   │   ├── CMakeLists.txt
│   │   │   │   │   ├── include/
│   │   │   │   │   │   ├── swarm_classifier.hpp  # Klasyfikator rojenia
│   │   │   │   │   │   ├── disease_detector.hpp  # Detektor chorób
│   │   │   │   │   │   └── anomaly_detection.hpp # Detekcja anomalii
│   │   │   │   │   └── src/
│   │   │   │   │       ├── swarm_classifier.cpp
│   │   │   │   │       ├── disease_detector.cpp
│   │   │   │   │       └── anomaly_detection.cpp
│   │   │   │   └── real_time_kernel/
│   │   │   │       ├── CMakeLists.txt
│   │   │   │       └── rt_scheduler.cpp          # Real-time scheduler
│   │   │   │
│   │   │   └── bash/                   # Skrypty Bash
│   │   │       ├── system/
│   │   │       │   ├── install.sh          # Instalacja systemu
│   │   │       │   ├── update.sh           # Aktualizacja oprogramowania
│   │   │       │   ├── backup.sh           # Backup danych i konfiguracji
│   │   │       │   ├── restore.sh          # Przywracanie z backupu
│   │   │       │   ├── health_check.sh     # Sprawdzenie zdrowia systemu
│   │   │       │   ├── log_rotation.sh     # Rotacja logów
│   │   │       │   ├── security_hardening.sh # Hardening bezpieczeństwa
│   │   │       │   └── factory_reset.sh    # Przywrócenie ustawień fabrycznych
│   │   │       ├── network/
│   │   │       │   ├── lte_setup.sh        # Konfiguracja LTE Aero2
│   │   │       │   ├── lte_monitor.sh      # Monitorowanie połączenia LTE
│   │   │       │   ├── lte_reconnect.sh    # Automatyczne reconnect
│   │   │       │   ├── firewall_setup.sh   # Konfiguracja iptables
│   │   │       │   ├── vpn_tunnel.sh       # VPN tunnel setup
│   │   │       │   └── bandwidth_test.sh   # Test przepustowości
│   │   │       ├── sensors/
│   │   │       │   ├── calibrate_scale.sh  # Kalibracja wagi
│   │   │       │   ├── test_microphone.sh  # Test mikrofonu
│   │   │       │   ├── read_all_sensors.sh # Odczyt wszystkich sensorów
│   │   │       │   └── sensor_diagnostics.sh # Diagnostyka sensorów
│   │   │       ├── services/
│   │   │       │   ├── apache_install.sh   # Instalacja Apache2
│   │   │       │   ├── dotnet_install.sh   # Instalacja .NET Core
│   │   │       │   ├── mqtt_broker.sh      # Instalacja Mosquitto
│   │   │       │   ├── database_init.sh    # Inicjalizacja bazy danych
│   │   │       │   ├── start_all.sh        # Start wszystkich usług
│   │   │       │   ├── stop_all.sh         # Stop wszystkich usług
│   │   │       │   └── restart_failed.sh   # Restart failed services
│   │   │       ├── deployment/
│   │   │       │   ├── deploy_prod.sh      # Deploy produkcyjny
│   │   │       │   ├── deploy_staging.sh   # Deploy staging
│   │   │       │   ├── rollback.sh         # Rollback wersji
│   │   │       │   └── version_check.sh    # Sprawdzenie wersji
│   │   │       └── utilities/
│   │   │           ├── disk_cleanup.sh     # Czyszczenie dysku
│   │   │           ├── memory_monitor.sh   # Monitor pamięci
│   │   │           ├── temperature_log.sh  # Logowanie temperatur CPU
│   │   │           ├── uptime_report.sh    # Raport uptime
│   │   │           └── generate_cert.sh    # Generowanie certyfikatów SSL
│   │   │
│   │   ├── config/
│   │   │   ├── apache2/
│   │   │   │   ├── 000-default.conf    # Apache virtual host config
│   │   │   │   ├── ssl.conf            # SSL/TLS configuration
│   │   │   │   └── htpasswd            # Basic auth passwords
│   │   │   ├── systemd/
│   │   │   │   ├── apiaryguard-core.service
│   │   │   │   ├── apiaryguard-worker.service
│   │   │   │   ├── apiaryguard-webapi.service
│   │   │   │   ├── mosquitto.service.override
│   │   │   │   └── lte-watchdog.service
│   │   │   ├── network/
│   │   │   │   ├── interfaces          # Network interfaces config
│   │   │   │   ├── wpa_supplicant.conf # WiFi config (backup)
│   │   │   │   └── chat-script-aero2   # PPP chat script for Aero2
│   │   │   ├── application/
│   │   │   │   ├── appsettings.Production.json
│   │   │   │   ├── appsettings.Development.json
│   │   │   │   ├── logging.json
│   │   │   │   └── serilog.config
│   │   │   └── database/
│   │   │       ├── schema.sql
│   │   │       ├── indexes.sql
│   │   │       └── seed_data.sql
│   │   │
│   │   ├── tests/
│   │   │   ├── unit/
│   │   │   │   ├── SensorTests.cs
│   │   │   │   ├── ActuatorTests.cs
│   │   │   │   └── ModelTests.cs
│   │   │   ├── integration/
│   │   │   │   ├── ApiIntegrationTests.cs
│   │   │   │   └── DatabaseIntegrationTests.cs
│   │   │   └── performance/
│   │   │       ├── LoadTests.cs
│   │   │       └── StressTests.cs
│   │   │
│   │   ├── scripts/
│   │   │   ├── build.sh
│   │   │   ├── run_tests.sh
│   │   │   └── package.sh
│   │   │
│   │   └── Dockerfile                  # Containerization
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
│   │       └── suppliers.md
│   │
│   └── electrical/                     # Schematy elektryczne
│       ├── schematics/
│       │   ├── main_wiring_scheme.pdf
│       │   ├── arduino_nano_schematic.pdf
│       │   ├── sensor_interface.pdf
│       │   └── actuator_driver.pdf
│       ├── pcb/
│       │   ├── sensor_board.kicad_pcb
│       │   ├── actuator_board.kicad_pcb
│       │   └── gerbers/
│       └── wiring_diagrams/
│           ├── power_distribution.png
│           ├── signal_routing.png
│           └── grounding_scheme.png
│
├── software/                           # Oprogramowanie wysokiego poziomu
│   ├── web_dashboard/                  # Frontend aplikacji webowej
│   │   ├── src/
│   │   │   ├── components/
│   │   │   ├── pages/
│   │   │   ├── services/
│   │   │   └── styles/
│   │   ├── package.json
│   │   └── webpack.config.js
│   │
│   ├── mobile_app/                     # Aplikacja mobilna
│   │   ├── flutter/
│   │   │   ├── lib/
│   │   │   ├── pubspec.yaml
│   │   │   └── ...
│   │
│   ├── cloud_services/                 # Usługi chmurowe
│   │   ├── aws_lambda/
│   │   ├── azure_functions/
│   │   └── data_pipeline/
│   │
│   └── analytics_engine/               # Silnik analityczny
│       ├── jupyter_notebooks/
│       ├── ml_models/
│       └── training_data/
│
├── data/                               # Dane i konfiguracje runtime
│   ├── databases/
│   │   ├── sqlite/                     # Local SQLite database
│   │   └── backups/                    # Automated backups
│   ├── logs/
│   │   ├── application/
│   │   ├── system/
│   │   └── access/
│   ├── cache/
│   └── uploads/
│
├── tools/                              # Narzędzia deweloperskie
│   ├── simulators/
│   │   ├── hive_simulator.py           # Symulator ula (testy)
│   │   ├── sensor_emulator.cpp         # Emulator sensorów
│   │   └── network_simulator.sh        # Symulator sieci LTE
│   ├── debuggers/
│   │   ├── serial_monitor.sh
│   │   ├── mqtt_explorer.sh
│   │   └── log_analyzer.sh
│   └── generators/
│       ├── config_generator.sh
│       ├── certificate_generator.sh
│       └── mock_data_generator.cpp
│
├── ci_cd/                              # Continuous Integration/Deployment
│   ├── github_actions/
│   │   ├── build.yml
│   │   ├── test.yml
│   │   └── deploy.yml
│   ├── jenkins/
│   │   └── Jenkinsfile
│   └── scripts/
│       ├── pre_commit_checks.sh
│       └── post_deploy_verify.sh
│
├── third_party/                        # Biblioteki zewnętrzne
│   ├── arduino_libs/
│   ├── dotnet_packages/
│   └── cpp_modules/
│
└── misc/
    ├── branding/
    │   ├── logo.svg
    │   ├── icons/
    │   └── styleguide.md
    ├── legal/
    │   ├── privacy_policy.md
    │   ├── terms_of_service.md
    │   └── compliance/
    └── community/
        ├── contributing.md
        ├── code_of_conduct.md
        └── faq.md
```

---

