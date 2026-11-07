# HiveMonitor Desktop — C++ / Qt 6

Natywny klient desktopowy C++ oparty na Qt 6. Nie używa PySide, PyQt, Pythona ani PyInstaller.

## Architektura

- Zakładka **Monitoring** osadza istniejący WebUI przez Qt WebEngine.
- Zakładka **Diagnostyka** odpytuje te same endpointy pośredniczącego API PHP, z których korzysta WebUI: `api.php?endpoint=health` i `api.php?endpoint=status`.
- WebUI i TUI pozostają klientami tego samego kolektora C++ i tej samej bazy danych. Desktop nie uruchamia drugiego kolektora ani nie ma osobnej logiki pomiarowej.
- Diagnostyka przechodzi przez proxy PHP WebUI; aplikacja nie łączy się bezpośrednio z portem kolektora 8080.
- Nawigacja wbudowanego WebEngine jest ograniczona do tego samego hosta i portu oraz HTTP/HTTPS. Weryfikacja certyfikatów TLS pozostaje włączona.

## Wymagania

- CMake 3.21+
- kompilator C++17
- Qt 6.4+ z modułami Widgets, Network i WebEngineWidgets

### Linux (Ubuntu 22.04+)

    sudo apt-get install cmake ninja-build g++ qt6-base-dev qt6-webengine-dev qt6-webengine-dev-tools
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

Uruchomienie: `./build/HiveMonitorDesktop`.

### Windows Server 2022 / 2025

Zainstaluj Qt 6 dla MSVC 2022 wraz z modułem Qt WebEngine oraz Visual Studio 2022 C++ Build Tools. Następnie w Developer PowerShell:

    cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
    cmake --build build --config Release --parallel
    windeployqt --release --dir deploy build/Release/HiveMonitorDesktop.exe

Uruchom `deploy/HiveMonitorDesktop.exe`.

## Połączenie

Podaj adres wdrożonego WebUI, np. `https://192.168.1.100/apiary/`. Adres jest zapamiętywany w ustawieniach użytkownika. W produkcji używaj HTTPS z poprawnym certyfikatem.

Serwer Raspberry Pi/Linux nadal uruchamia kolektor, bazę danych, PHP i WebUI. Desktop jest klientem sieciowym, nie zamiennikiem serwera.
