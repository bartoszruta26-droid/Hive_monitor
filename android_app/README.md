# ApiaryGuard - Aplikacja Android do Monitorowania Uli

## Opis
ApiaryGuard to prosta aplikacja na system Android umożliwiająca monitorowanie stanu uli pszczelich. Aplikacja wyświetla dane takie jak temperatura, wilgotność, waga oraz status każdego ula.

## Funkcje
- 📊 **Podgląd danych z uli**: Temperatura, wilgotność, waga
- 🔔 **Statusy uli**: OK, UWAGA, ALERT (kolorystyczne oznaczenia)
- 🔄 **Odświeżanie danych**: Pull-to-refresh i przycisk FAB
- 📱 **Nowoczesny UI**: Material Design z CardView
- ⏰ **Ostatnia aktualizacja**: Czas ostatniego odczytu

## Struktura projektu

```
android_app/
├── app/
│   ├── src/main/
│   │   ├── java/com/apiaryguard/app/
│   │   │   ├── MainActivity.java       # Główna aktywność
│   │   │   ├── Apiary.java             # Model danych ula
│   │   │   ├── ApiaryAdapter.java      # Adapter RecyclerView
│   │   │   └── MainViewModel.java      # ViewModel MVVM
│   │   ├── res/
│   │   │   ├── layout/
│   │   │   │   ├── activity_main.xml   # Layout głównej aktywności
│   │   │   │   └── item_apiary.xml     # Layout pojedynczego ula
│   │   │   ├── values/
│   │   │   │   ├── strings.xml         # Stringi
│   │   │   │   ├── colors.xml          # Kolory
│   │   │   │   └── themes.xml          # Motyw aplikacji
│   │   │   └── drawable/               # Zasoby graficzne
│   │   └── AndroidManifest.xml         # Manifest aplikacji
│   ├── build.gradle                    # Konfiguracja modułu app
│   └── proguard-rules.pro              # Reguły ProGuard
├── gradle/wrapper/
│   └── gradle-wrapper.properties       # Konfiguracja Gradle Wrapper
├── build.gradle                        # Konfiguracja projektu
├── settings.gradle                     # Ustawienia projektu
├── gradle.properties                   # Właściwości Gradle
└── gradlew                             # Skrypt uruchomieniowy
```

## Wymagania
- Android Studio Arctic Fox lub nowszy
- MinSDK: 24 (Android 7.0)
- TargetSDK: 34 (Android 14)
- Java 8+

## Instalacja

### 1. Otwórz projekt w Android Studio
```
File → Open → Wybierz folder android_app
```

### 2. zsynchronizuj Gradle
Poczekaj aż Android Studio zakończy synchronizację zależności.

### 3. Uruchom aplikację
- Podłącz urządzenie z Androidem lub uruchom emulator
- Kliknij **Run** (zielony trójkąt)

## Architektura

Aplikacja korzysta z architektury **MVVM** (Model-View-ViewModel):

- **Model** (`Apiary.java`): Reprezentuje dane ula
- **View** (`MainActivity.java`, layouty XML): Wyświetla dane użytkownikowi
- **ViewModel** (`MainViewModel.java`): Zarządza danymi i logiką biznesową

## Integracja z API

Aby podłączyć aplikację do rzeczywistego API:

1. Dodaj interfejs Retrofit w pliku `ApiService.java`:
```java
public interface ApiService {
    @GET("apiaries")
    Call<List<Apiary>> getApiaries();
}
```

2. Zaktualizuj `MainViewModel.java` aby używał Retrofit zamiast danych testowych.

3. Dodaj adres URL API w `gradle.properties` lub pliku konfiguracyjnym.

## Dane demonstracyjne

Aplikacja generuje losowe dane dla 3 uli:
- Ul nr 1 - Pasieka Główna
- Ul nr 2 - Pasieka Główna  
- Ul nr 3 - Pasieka Leśna

## Rozszerzenia

Możliwe rozszerzenia aplikacji:
- [ ] Połączenie z backendem przez REST API
- [ ] Powiadomienia push o alertach
- [ ] Wykresy historyczne (MPAndroidChart)
- [ ] Tryb offline z bazą Room
- [ ] Dodawanie/edycja uli
- [ ] Ustawienia powiadomień
- [ ] Eksport danych do CSV

## Licencja

Projekt dostępny na licencji MIT.

## Autor

ApiaryGuard Team
