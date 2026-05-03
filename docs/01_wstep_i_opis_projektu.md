## 1. Wstęp i Opis Projektu

**ApiaryGuard Pro** to zaawansowany, skalowalny system monitoringu i zarządzania pasieką klasy enterprise, zaprojektowany do pracy w trudnych warunkach terenowych. System pozwala na centralną obsługę **wielu uli** (multi-hive) za pośrednictwem jednego serwera Apache2 hostowanego na Raspberry Pi 2, który komunikuje się z rozproszonymi jednostkami końcowymi opartymi o mikrokontrolery Arduino Nano.

### Kluczowe Założenia Projektowe:
*   **Multi-Tenancy:** Jeden serwer zbiera dane z dziesiątek uli, identyfikując każdą jednostkę po unikalnym ID sprzętowym.
*   **Connectivity:** Hybrydowa łączność wykorzystująca darmową sieć LTE **Aero2** (SIM free) jako główny kanał transmisji danych zdalnych oraz Ethernet PoE dla lokalnej komunikacji wysokiej przepustowości (kamery, aktualizacje).
*   **Stack Technologiczny:** Ścisłe przestrzeganie zakazu używania Pythona. Cały stack oparty jest o wydajne języki kompilowane: **C++** (firmware Arduino), **C#** (.NET Core backend logic), **Bash** (skrypty systemowe Linux) oraz **SQL** (bazy danych).
*   **Zaawansowana Diagnostyka:** Pełny zestaw sensorów biometrycznych ula (waga, dźwięk, wibracje, temperatura, wilgotność) wsparty aktywnymi efektorami (grzanie, wentylacja, podawanie leków).
*   **Monitoring Wizyjny:** Zintegrowana kamera PoE wykonująca zdjęcia co 60 sekund w celu analizy aktywności wylotowej i wykrywania intruzów.
*   **AI-Driven:** Rdzeń decyzyjny oparty o agenta **Qwen AI**, zapewniający predykcję rójki, diagnozę chorób i autonomiczne reakcje.

---

