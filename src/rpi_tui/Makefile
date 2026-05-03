# Makefile dla APIARY Guard Logger C++
# Kompilacja na Raspberry Pi

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

# Pliki źródłowe
SRCS = apiary_logger.cpp
OBJS = $(SRCS:.cpp=.o)

# Plik wynikowy
TARGET = apiary_logger

# Tryb testowy (standalone)
TEST_TARGET = apiary_logger_test
TEST_FLAGS = -DSTANDALONE_TEST

# Domyślny cel
all: $(TARGET) $(TEST_TARGET)

# Budowanie biblioteki obiektowej
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Budowanie wersji produkcyjnej (bez main)
$(TARGET): $(OBJS)
	@echo "Budowanie biblioteki loggera..."
	@echo "Logger zbudowany jako plik nagłówkowy - użyj w swoim projekcie"

# Budowanie wersji testowej standalone
$(TEST_TARGET): apiary_logger.cpp
	@echo "Budowanie wersji testowej..."
	$(CXX) $(CXXFLAGS) $(TEST_FLAGS) $< -o $@ $(LDFLAGS)
	@echo "Gotowe: ./$(TEST_TARGET)"

# Czyszczenie
clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET) *.log

# Instalacja (wymaga sudo)
install: $(TEST_TARGET)
	@echo "Instalowanie..."
	sudo mkdir -p /var/log/apiaryguard
	sudo mkdir -p /etc/apiaryguard
	sudo mkdir -p /var/lib/apiaryguard
	sudo chmod 777 /var/log/apiaryguard
	sudo cp $(TEST_TARGET) /usr/local/bin/apiary-logger
	@echo "Zainstalowano jako /usr/local/bin/apiary-logger"

# Odinstalowanie
uninstall:
	sudo rm -f /usr/local/bin/apiary-logger
	@echo "Odinstalowano"

# Testowanie
test: $(TEST_TARGET)
	@echo "Uruchamianie testów..."
	./$(TEST_TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: clean $(TEST_TARGET)

# Pomoc
help:
	@echo "APIARY Guard Logger - Makefile"
	@echo ""
	@echo "Cele:"
	@echo "  all       - Buduj wszystkie wersje (domyślne)"
	@echo "  clean     - Wyczyść pliki tymczasowe"
	@echo "  install   - Zainstaluj w systemie"
	@echo "  uninstall - Odinstaluj z systemu"
	@echo "  test      - Uruchom testy"
	@echo "  debug     - Build z symbolami debugowania"
	@echo "  help      - Pokaż tę pomoc"
	@echo ""
	@echo "Przykładowe użycie:"
	@echo "  make              # Buduj"
	@echo "  make test         # Testuj"
	@echo "  sudo make install # Instaluj"

.PHONY: all clean install uninstall test debug help
