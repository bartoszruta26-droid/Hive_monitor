## 🔧 Konserwacja i Rozwiązywanie Problemów

### Harmonogram Konserwacji

**Codziennie:**
- [ ] Sprawdzenie statusu online (dashboard)
- [ ] Przegląd aktywnych alertów
- [ ] Weryfikacja poziomu baterii backup

**Co Tydzień:**
- [ ] Inspekcja fizyczna uli (tradycyjna)
- [ ] Czyszczenie wylotków z martwych pszczół
- [ ] Sprawdzenie mocowania sensorów
- [ ] Test ręcznego uruchomienia efektorów

**Co Miesiąc:**
- [ ] Kalibracja wagi (test znanym ciężarem)
- [ ] Czyszczenie mikrofonu z kurzu
- [ ] Backup konfiguracji
- [ ] Update systemu operacyjnego
- [ ] Przegląd logów pod kątem warningów

**Co Kwartalnie:**
- [ ] Pełna diagnostyka systemu (health_check.sh)
- [ ] Test procedury disaster recovery
- [ ] Wymiana filtrów powietrza (jeśli dotyczy)
- [ ] Sprawdzenie uszczelek obudowy (waterproofing)
- [ ] Audyt bezpieczeństwa (logs, access)

**Sezonowo:**
- [ ] Przed sezonem: Pełny przegląd techniczny
- [ ] Po sezonie: Konserwacja zimowa, demontaż części sensorów
- [ ] Zima: Tryb low-power, minimalny sampling

### Troubleshooting Guide

#### Problem: Brak połączenia LTE

**Diagnostyka:**
```bash
# Sprawdź czy modem jest wykrywany
lsusb | grep -i huawei

# Status połączenia PPP
ip addr show ppp0
systemctl status ppp@aero2

# Logi modemu
tail -f /var/log/ppp/aero2.log

# Siła sygnału
mmcli -m 0 --signal-get
```

**Rozwiązania:**
1. Restart modemu: `usb_modeswitch -v 0x12d1 -p 0x1506 -R`
2. Reconnect: `poff aero2 && pon aero2`
3. Sprawdzenie konta Aero2: `*101#`
4. Zmiana lokalizacji anteny zewnętrznej

#### Problem: Nieprawidłowe odczyty wagi

**Diagnostyka:**
```bash
# Test bezpośredni HX711
i2cdetect -y 1
i2cget -y 1 0x48

# Uruchom diagnostykę
./sensor_diagnostics.sh --sensor weight

# Sprawdź kalibrację
sqlite3 data.db "SELECT * FROM calibration_history ORDER BY date DESC LIMIT 5;"
```

**Rozwiązania:**
1. Ponowna kalibracja: `calibrate_scale.sh --known-weight 10.0`
2. Sprawdzenie połączeń kablowych tensometrów
3. Izolacja od wibracji (podkładki gumowe)
4. Wymiana HX711 jeśli uszkodzony

#### Problem: Przegrzewanie Raspberry Pi

**Diagnostyka:**
```bash
# Temperatura CPU
vcgencmd measure_temp

# Throttling status
vcgencmd get_throttled

# Process load
top -bn1 | head -20
```

**Rozwiązania:**
1. Dodanie heatsink + fan
2. Obniżenie clock speed: `over_voltage=-2` w config.txt
3. Sprawdzenie obciążenia procesami
4. Przeniesienie w cieńsze miejsce

#### Problem: Usługa nie startuje

**Diagnostyka:**
```bash
# Status usługi
systemctl status apiaryguard-core

# Logi journal
journalctl -u apiaryguard-core -n 50 --no-pager

# Ręczny start dla debugowania
/opt/apiaryguard/publish/core/ApiaryGuard.Core
```

**Rozwiązania:**
1. Sprawdzenie dependencies: `systemctl list-dependencies apiaryguard-core`
2. Weryfikacja connection string do bazy danych
3. Sprawdzenie uprawnień do plików
4. Przywrócenie z backupu konfiguracji

### Backup i Restore

**Tworzenie Backupu:**
```bash
./backup.sh --full --destination /mnt/external_drive/backups
# Lub automatycznie codziennie o 3:00
```

**Przywracanie:**
```bash
./restore.sh --source /mnt/external_drive/backups/backup_20240115.tar.gz --verify
```

**Zawartość Backupu:**
- Baza danych SQLite
- Konfiguracje aplikacji (appsettings.json)
- Konfiguracje systemowe (network, Apache, MQTT)
- Firmware Raspberry Pi Pico (UF2)
- Nagrania audio (ostatnie 30 dni)
- Logi (ostatnie 7 dni)

---

