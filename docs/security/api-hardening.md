# Utwardzenie API WebUI

## Zmienne środowiskowe

| Zmienna | Domyślnie | Znaczenie |
|---|---|---|
| `APIARY_ENVIRONMENT` | development | Ustaw `production` za terminatorem TLS. |
| `APIARY_DEBUG` | `false` | Włącza szczegółowe komunikaty diagnostyczne. Nie włączać publicznie. |
| `APIARY_LOG_REQUESTS` | `true` | Włącza dziennik żądań. |
| `APIARY_CORS_ORIGIN` | `http://localhost` | Jeden dozwolony origin; nie używać `*`. |
| `APIARY_DEMO_MODE` | `false` | Jawnie włącza syntetyczne dane demonstracyjne. |
| `APIARY_API_KEY` | brak | Wymagany dla żądań POST/PUT/DELETE. Gdy brak, zapis jest zamknięty. |

Klucz API należy dostarczać przez menedżer sekretów lub uprawniony plik środowiskowy poza repozytorium. Nie umieszczać go w kodzie, plikach wersjonowanych ani aplikacji mobilnej.

## Zmiana zachowania

- Brak kolektora zwraca HTTP 503 zamiast domyślnie podawać fikcyjne pomiary.
- Dane demo można włączyć wyłącznie przez `APIARY_DEMO_MODE=true`; odpowiedź zawiera `mode=demo`, `data_quality=DEMO` i ostrzeżenie.
- Operacje dodawania i sterowania sensorami/efektorami zwracają HTTP 501, dopóki rzeczywista implementacja nie będzie gotowa. Backend nie potwierdza już fikcyjnego sukcesu.
- Operacje POST/PUT/DELETE wymagają `X-API-Key`. Brak skonfigurowanego klucza blokuje zapis kodem 503.
- CORS dopuszcza wyłącznie dokładnie skonfigurowany origin.

## Ważne ograniczenia

To jest warstwa ograniczająca ryzyko, a nie kompletna autoryzacja produkcyjna. Wspólny klucz API nie zastępuje tożsamości użytkowników, ról, rotacji tokenów, ochrony przed CSRF ani uwierzytelniania urządzeń. Nie należy wystawiać kolektora C++ bezpośrednio do niezaufanej sieci. Przed wdrożeniem należy uruchomić TLS na zaufanym reverse proxy, skonfigurować sekret i przetestować integrację panelu.

Endpointy `history`, `logs`, `sensors` i `effectors` zależą od implementacji po stronie kolektora. Jeżeli kolektor nie dostarcza odpowiedzi, API zwraca jawny błąd zamiast udawać, że posiada dane.
