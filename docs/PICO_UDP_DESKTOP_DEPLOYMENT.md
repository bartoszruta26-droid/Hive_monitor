# Pico UDP i wdrożenie Desktop GUI

## Kontrakt telemetryczny Pico

Firmware wysyła datagram UDP na adres RPI_IP i port UDP_PORT z src/pico_refactored/include/config.h. Payload ma jawny identyfikator ula i nazwy pól zgodne z kolektorem:

    {"hive_id":"UL-001","data_source":"pico","temp":24.5,"hum":65.2,"weight_raw":450000,"voc":35,"nox":12,"co2_available":true,"co2":450}

Liczby są przykładowe i opisują wyłącznie format. Firmware pomija pole co2, gdy pomiar CO2 jest niedostępny; nie wpisuje fikcyjnego zera. Pole nox jest zachowywane w surowym JSON i walidowane; nie jest obecnie osobną kolumną podstawowej struktury HiveData. HX711 w obecnym firmware zwraca surowe zliczenia ADC, dlatego wysyłane jest weight_raw, a nie weight w kilogramach. Do prezentacji masy w kg potrzebna jest rzeczywista kalibracja i jawne przeliczenie.

Dla każdego fizycznego ula ustaw unikalne APIARY_HIVE_ID w konfiguracji firmware. Domyślna wartość to UL-001.

## Konfiguracja sieci

1. Ustaw RPI_IP w konfiguracji Pico na rzeczywisty, stały adres LAN Raspberry Pi.
2. Ustaw APIARY_UDP_BIND_ADDRESS w środowisku usługi kolektora na konkretny adres LAN Raspberry Pi, np. 192.168.1.100.
3. Zachowaj domyślne 127.0.0.1, jeżeli Pico nie ma łączyć się przez sieć. Nie używaj 0.0.0.0 bez uzasadnionej potrzeby.
4. Zezwól na UDP/5005 w zaporze wyłącznie z podsieci lub adresu urządzenia Pico.
5. Port HTTP API 8080 pozostaw lokalnie/reverse proxy, chyba że polityka sieciowa wymaga inaczej.

Przykład wpisu w sekcji Service jednostki systemd:

    Environment=APIARY_UDP_BIND_ADDRESS=192.168.1.100

Po zmianie konfiguracji:

    sudo systemctl daemon-reload
    sudo systemctl restart apiary-collector
    sudo ss -lunp | grep 5005

Na Raspberry Pi można obserwować pakiety:

    sudo tcpdump -ni eth0 udp port 5005

## Test kontraktu

    make -C src/rpi_tui test-pico-udp

Test sprawdza parser JSON i walidator kolektora, w tym wymagany hive_id, data_source, pola pomiarowe, dostępność CO2 i zakres NOx.

## Desktop GUI

Desktop GUI jest klientem istniejącego WebUI. Serwer Raspberry Pi nadal uruchamia kolektor, bazę danych i PHP/WebUI. Na komputerze Linux lub Windows Server 2022/2025 uruchom HiveMonitorDesktop i podaj adres WebUI, np. https://192.168.1.100/apiary/.

Pakiety są budowane przez workflow Desktop GUI build na Ubuntu 22.04, Windows Server 2022 i Windows Server 2025. Artefakty workflow są pakietami testowymi; przed wdrożeniem produkcyjnym zweryfikuj je na docelowych maszynach.
