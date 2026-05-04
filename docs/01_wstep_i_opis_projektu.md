## 1. Wstęp i Opis Projektu

**ApiaryGuard Pro** to zaawansowany, skalowalny system monitoringu i zarządzania pasieką klasy enterprise, zaprojektowany do pracy w trudnych warunkach terenowych. System pozwala na centralną obsługę **wielu uli** (multi-hive) za pośrednictwem jednego serwera Apache2 hostowanego na Raspberry Pi 2, który komunikuje się przez HTTP API z rozproszonymi jednostkami końcowymi opartymi o mikrokontrolery **Raspberry Pi Pico** (RP2040).

### Kluczowe Założenia Projektowe:
*   **Multi-Tenancy:** Jeden serwer zbiera dane z dziesiątek uli, identyfikując każdą jednostkę po unikalnym ID sprzętowym.
*   **Connectivity:** Hybrydowa łączność wykorzystująca darmową sieć LTE **Aero2** (SIM free) jako główny kanał transmisji danych zdalnych oraz Ethernet PoE dla lokalnej komunikacji wysokiej przepustowości (kamery, aktualizacje).
*   **Stack Technologiczny:** Ścisłe przestrzeganie zakazu używania Pythona. Cały stack oparty jest o wydajne języki kompilowane: **C++** (firmware Raspberry Pi Pico), **Bash** (skrypty systemowe Linux, logika backendowa na RPi2), **C++** (aplikacje TUI/GUI na RPi2) oraz **SQL** (bazy danych).
*   **Architektura Komunikacji:** Raspberry Pi 2 pełni rolę serwera HTTP z interfejsem GUI/TUI, komunikującego się przez REST API z Raspberry Pi Pico w każdym ulu.
*   **Zaawansowana Diagnostyka:** Pełny zestaw sensorów biometrycznych ula (waga, dźwięk, wibracje, temperatura, wilgotność) wsparty aktywnymi efektorami (grzanie, wentylacja, podawanie leków).
*   **Monitoring Wizyjny:** Zintegrowana kamera PoE wykonująca zdjęcia co 60 sekund w celu analizy aktywności wylotowej i wykrywania intruzów.
*   **AI-Driven (Planowane):** Integracja z agentem **Qwen AI**, zapewniającym predykcję rójki, diagnozę chorób i autonomiczne reakcje.

---

