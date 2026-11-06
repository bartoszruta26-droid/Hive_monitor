# Bezpieczeństwo nasłuchiwania kolektora

## Domyślne zachowanie

Kolektor UDP i serwer HTTP nasłuchują domyślnie wyłącznie na `127.0.0.1`.
Chroni to przed przypadkowym wystawieniem nieautoryzowanego API na wszystkie
interfejsy sieciowe. Adresy wiązania są walidowane jako IPv4 z zakresów
prywatnych RFC 1918, loopback lub link-local; `0.0.0.0`, adresy publiczne i niepoprawne
wartości powodują przerwanie inicjalizacji odpowiedniego gniazda.

## Konfiguracja pracy w sieci lokalnej

Jeżeli urządzenia pomiarowe wysyłają datagramy przez LAN, ustaw adres konkretnego
interfejsu Raspberry Pi, np.:

```sh
export APIARY_UDP_BIND_ADDRESS=192.168.1.20
```

Jeżeli interfejs HTTP ma być dostępny z innego hosta w zaufanej sieci, ustaw
oddzielnie:

```sh
export APIARY_HTTP_BIND_ADDRESS=192.168.1.20
```

Adres musi być przypisany do interfejsu hosta i należeć do zakresu prywatnego.
Nie używaj `0.0.0.0`. Przy udostępnieniu HTTP w LAN wymagane jest ograniczenie
dostępu regułami zapory do zaufanych hostów. Serwer kolektora nie zastępuje
uwierzytelniania warstwy aplikacyjnej.

Jeżeli WebUI PHP łączy się z kolektorem przez adres inny niż localhost, ustaw
`APIARY_COLLECTOR_HOST` w środowisku PHP na ten sam adres interfejsu HTTP.
Domyślna konfiguracja WebUI używa `localhost`.

## Ograniczenia i statusy endpointów

- `GET /health` jest kontrolą żywotności procesu (liveness), a nie gwarancją,
  że sensory lub historia danych są gotowe.
- `/api/sensors`, `/api/effectors`, `/api/history` i `/api/logs` zwracają
  HTTP 501, dopóki ich rzeczywista obsługa nie zostanie zaimplementowana.
  Nie są zwracane syntetyczne, puste zestawy danych jako wynik sukcesu.
- Odbiór UDP odrzuca datagramy oznaczone przez system jako obcięte.
- Rate limiter usuwa stare wpisy okresowo i ogranicza mapę do 4096 śledzonych
  adresów; przy wyczerpaniu limitu nowe adresy są odrzucane (fail closed).

## Testy

Uruchom:

```sh
make -C src/rpi_tui test-network
make -C src/rpi_tui test-http
```

Testy obejmują granice rozmiaru datagramu, flagę `MSG_TRUNC`, walidację
adresów wiązania i kody HTTP endpointów niezaimplementowanych. Testy sprzętowe
oraz testy rzeczywistego ruchu z urządzeń Pico/Nano pozostają odrębnym etapem.
