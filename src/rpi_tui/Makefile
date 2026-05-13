# Makefile dla APIARY Guard - Logger, Baza Danych i Kolektor Danych Uli
# Kompilacja na Raspberry Pi 2

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread -lsqlite3

# Pliki źródłowe
LOGGER_SRCS = apiary_logger.cpp
DATABASE_SRCS = apiary_database.cpp
COLLECTOR_SRCS = apiary_collector.cpp
ALL_SRCS = $(LOGGER_SRCS) $(DATABASE_SRCS) $(COLLECTOR_SRCS)

# Pliki wynikowe
LOGGER_TARGET = apiary_logger
COLLECTOR_TARGET = apiary_collector
TEST_TARGET = apiary_logger_test

# Tryb testowy (standalone)
TEST_FLAGS = -DSTANDALONE_TEST

# Domyślny cel
all: $(LOGGER_TARGET) $(COLLECTOR_TARGET) $(TEST_TARGET)

# Budowanie biblioteki obiektowej loggera
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Budowanie wersji produkcyjnej loggera
$(LOGGER_TARGET): apiary_logger.o
	@echo "Budowanie biblioteki loggera..."
	@echo "Logger zbudowany jako plik nagłówkowy - użyj w swoim projekcie"

# Budowanie kolektora danych z uli (z obsługą bazy danych)
$(COLLECTOR_TARGET): $(ALL_SRCS) apiary_database.h apiary_logger.h
	@echo "Budowanie kolektora danych uli z bazą danych SQLite..."
	$(CXX) $(CXXFLAGS) -DCOLLECTOR_BUILD $(COLLECTOR_SRCS) $(DATABASE_SRCS) $(LOGGER_SRCS) -o $@ $(LDFLAGS)
	@echo "Gotowe: ./$(COLLECTOR_TARGET)"

# Budowanie wersji testowej standalone loggera
$(TEST_TARGET): apiary_logger.cpp
	@echo "Budowanie wersji testowej loggera..."
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $< -o $@ $(LDFLAGS)
	@echo "Gotowe: ./$(TEST_TARGET)"

# Czyszczenie
clean:
	rm -f *.o $(LOGGER_TARGET) $(COLLECTOR_TARGET) $(TEST_TARGET) *.log

# Instalacja (wymaga sudo)
install: $(COLLECTOR_TARGET) $(TEST_TARGET)
	@echo "Instalowanie..."
	sudo mkdir -p /var/log/apiaryguard
	sudo mkdir -p /etc/apiaryguard
	sudo mkdir -p /var/lib/apiaryguard
	sudo chmod 777 /var/log/apiaryguard
	sudo cp $(COLLECTOR_TARGET) /usr/local/bin/apiary-collector
	sudo cp $(TEST_TARGET) /usr/local/bin/apiary-logger
	sudo cp apiary_tui.sh /usr/local/bin/apiary-tui
	sudo chmod +x /usr/local/bin/apiary-tui
	@echo "Zainstalowano: apiary-collector, apiary-logger, apiary-tui"

# Odinstalowanie
uninstall:
	sudo rm -f /usr/local/bin/apiary-collector
	sudo rm -f /usr/local/bin/apiary-logger
	sudo rm -f /usr/local/bin/apiary-tui
	@echo "Odinstalowano"

# Testowanie
test: $(TEST_TARGET)
	@echo "Uruchamianie testów loggera..."
	./$(TEST_TARGET)

# Testowanie kolektora w trybie symulacji
test-collector: $(COLLECTOR_TARGET)
	@echo "Uruchamianie kolektora w trybie symulacji (Ctrl+C aby zatrzymać)..."
	./$(COLLECTOR_TARGET) --sim &
	sleep 5
	kill %1 2>/dev/null || true
	@echo "Test zakończony"

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: clean $(COLLECTOR_TARGET) $(TEST_TARGET)

# Pomoc
help:
	@echo "APIARY Guard - Makefile"
	@echo ""
	@echo "Cele:"
	@echo "  all           - Buduj wszystkie wersje (domyślne)"
	@echo "  clean         - Wyczyść pliki tymczasowe"
	@echo "  install       - Zainstaluj w systemie"
	@echo "  uninstall     - Odinstaluj z systemu"
	@echo "  test          - Uruchom testy loggera"
	@echo "  test-collector- Testuj kolektor w trybie symulacji"
	@echo "  debug         - Build z symbolami debugowania"
	@echo "  help          - Pokaż tę pomoc"
	@echo ""
	@echo "Przykładowe użycie:"
	@echo "  make              # Buduj wszystko"
	@echo "  make test         # Testuj logger"
	@echo "  make test-collector # Testuj kolektor (symulacja)"
	@echo "  sudo make install # Instaluj"

.PHONY: all clean install uninstall test test-collector debug help
