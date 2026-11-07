# ApiaryGuard Desktop

Desktopowy klient istniejącego WebUI. Aplikacja osadza WebUI w oknie Qt WebEngine i łączy się z API oraz kolektorem działającym na serwerze Raspberry Pi/Linux. Nie tworzy osobnej bazy danych, drugiego kolektora ani fikcyjnych pomiarów.

## Obsługiwane systemy

- Linux x86_64 (budowanie na Ubuntu 22.04).
- Windows x86_64, w tym Windows Server 2022 i Windows Server 2025.

Pakiet uruchomieniowy jest budowany przez PyInstaller. Na komputerze klienckim nie trzeba instalować Pythona ani PHP. Serwer WebUI/API musi być dostępny w sieci.

## Uruchomienie ze źródeł

Python 3.11:

    python -m venv .venv
    # Linux: source .venv/bin/activate
    # Windows PowerShell: .venv\Scripts\Activate.ps1
    python -m pip install -r requirements.txt
    python desktop_gui.py

W polu „Adres WebUI” wpisz adres wdrożenia, np. https://192.168.1.100/apiary/, i wybierz „Połącz”. Adres zostaje zapamiętany w ustawieniach użytkownika. W środowisku produkcyjnym zalecany jest HTTPS z poprawnym certyfikatem.

## Budowanie pakietu

    pyinstaller --noconfirm --clean --windowed --name HiveMonitorDesktop --collect-all PySide6 --collect-all shiboken6 desktop_gui.py

Plik wynikowy znajduje się w katalogu dist/. Dla Windows należy budować na Windows; dla Linux — na Linux. Nie jest to kompilacja krzyżowa.

## Architektura i ograniczenia

- TUI nadal działa bez środowiska graficznego i pozostaje narzędziem diagnostycznym.
- Desktop GUI korzysta z tego samego WebUI, API i danych co przeglądarka.
- Desktop GUI nie steruje bezpośrednio GPIO, przekaźnikami, grzałką ani pompą.
- Klient wymaga działającego serwera WebUI; nie zastępuje kolektora Raspberry Pi.
- Nawigacja wbudowanej przeglądarki jest ograniczona do HTTP/HTTPS oraz about:. Dostęp do lokalnych plików nie jest włączony, a kontrola certyfikatów TLS pozostaje aktywna.
