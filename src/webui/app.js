/**
 * ApiaryGuard WebUI - JavaScript Application
 * Komunikacja z APIARY_COLLECTOR przez HTTP API JSON
 * 
 * Wszystkie żądania przechodzą przez ten sam origin i src/webui/api.php.
 * PHP proxy odpowiada za komunikację z kolektorem oraz politykę dostępu.
 */

// ============================================================================
// KONFIGURACJA
// ============================================================================
const CONFIG = {
    // Same-origin PHP proxy; never connect the browser directly to collector port 8080.
    apiBaseUrl: '',
    refreshInterval: 10000, // 10 sekund
    maxRetries: 3,
    endpoints: {
        status: 'api.php?endpoint=status',
        hives: 'api.php?endpoint=hives',
        health: 'api.php?endpoint=health',
        sensors: 'api.php?endpoint=sensors',
        effectors: 'api.php?endpoint=effectors',
        history: 'api.php?endpoint=history',
        logs: 'api.php?endpoint=logs',
        config: 'api.php?endpoint=config'
    },
    // Domyślne ustawienia (nadpisane przez loadConfig())
    settings: {
        refreshInterval: 10,
        alertThreshold: 35,
        language: 'pl',
        temperatureUnit: 'C',
        humidityUnit: '%',
        weightUnit: 'kg',
        dateFormat: 'DD.MM.YYYY',
        timeFormat: '24h',
        theme: 'light',
        showCharts: true,
        enableNotifications: true,
        emailNotifications: false,
        pushNotifications: false,
        tempMinAlert: 10,
        tempMaxAlert: 35,
        humidityMinAlert: 40,
        humidityMaxAlert: 70
    },
    debug: false // Zmień na true, aby włączyć logowanie w produkcji
};

// Stan aplikacji (encapsulated)
const AppState = {
    hives: [],
    sensors: [],
    effectors: [],
    logs: [],
    settings: {
        refreshInterval: 10,
        alertThreshold: 35
    },
    isConnected: false,
    lastRefresh: null,
    isLoading: true,
    isRefreshing: false, // Blokada race condition przy odświeżaniu
    retryCount: 0
};

// ============================================================================
// NARZĘDZIA POMOCNICZE
// ============================================================================

/**
 * Bezpieczne escapowanie danych przed wstawieniem do HTML (ochrona XSS)
 */
function escapeHtml(text) {
    if (typeof text !== 'string') return text;
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}

/**
 * Logowanie z poziomami (wyłączalne w produkcji, z ochroną przed rekurencją)
 */
let isLogging = false;

function logMessage(level, message, data = null) {
    if (!CONFIG.debug && level === 'debug') return;
    if (isLogging) return; // Zapobieganie rekurencji
    
    isLogging = true;
    try {
        const timestamp = new Date().toISOString();
        const prefix = `[${timestamp}] [${level.toUpperCase()}]`;
        
        const consoleMethod = level === 'error' ? 'error' : 
                             level === 'warn' ? 'warn' : 'log';
        
        if (data) {
            console[consoleMethod](prefix, message, data);
        } else {
            console[consoleMethod](prefix, message);
        }
    } finally {
        isLogging = false;
    }
}

/**
 * Opóźnienie (promise)
 */
function delay(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

// ============================================================================
// INICJALIZACJA
// ============================================================================
document.addEventListener('DOMContentLoaded', function() {
    loadLocalPreferences(); // Preferencje przeglądarki są lokalne, nie backendowe
    initTabs();
    initSettings();
    initModals(); // Inicjalizacja modali i formularzy
    loadDemoData(); // Czyści widok; dane demonstracyjne nie są ładowane automatycznie
    startAutoRefresh();
    checkCollectorConnection();
});

// ============================================================================
// ŁADOWANIE KONFIGURACJI Z BACKENDU
// ============================================================================
function loadLocalPreferences() {
    try {
        const stored = localStorage.getItem('apiaryguard.settings');
        if (stored) {
            const parsed = JSON.parse(stored);
            if (parsed && typeof parsed === 'object') {
                Object.assign(CONFIG.settings, parsed);
                const seconds = Number(CONFIG.settings.refreshInterval);
                if (Number.isFinite(seconds)) {
                    CONFIG.refreshInterval = Math.min(300, Math.max(5, seconds)) * 1000;
                }
            }
        }
    } catch (error) {
        logMessage('warn', 'Nie udało się odczytać lokalnych ustawień', error.message);
    }
}
function initTabs() {
    const tabs = document.querySelectorAll('.nav-tab');
    if (!tabs || tabs.length === 0) return; // Guard clause - check if elements exist
    
    tabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const tabId = this.getAttribute('data-tab');
            
            // Usuń active ze wszystkich
            tabs.forEach(t => t.classList.remove('active'));
            const tabContents = document.querySelectorAll('.tab-content');
            if (tabContents) {
                tabContents.forEach(c => c.classList.remove('active'));
            }
            
            // Dodaj active do klikniętej
            this.classList.add('active');
            const tabElement = document.getElementById(tabId);
            if (tabElement) {
                tabElement.classList.add('active');
            }
            
            // Odśwież dane w zależności od zakładki
            if (tabId === 'history') {
                loadHistoryData();
            } else if (tabId === 'logs') {
                loadLogs();
            }
        });
    });
}

// ============================================================================
// POŁĄCZENIE Z APIARY_COLLECTOR
// ============================================================================
async function checkCollectorConnection() {
    const statusEl = document.getElementById('collectorStatus');
    const progressEl = document.getElementById('connectionProgress');
    
    // Guard clause - sprawdź czy elementy istnieją
    if (!statusEl || !progressEl) {
        logMessage("warn", 'DOM elements not found for connection status - checking alternative elements');
        // Spróbuj zaktualizować przynajmniej wskaźnik połączenia w headerze
        updateConnectionStatus(false);
        return false;
    }
    
    try {
        progressEl.style.width = '50%';
        
        const response = await fetch(`${CONFIG.apiBaseUrl}${CONFIG.endpoints.health}`, {
            method: 'GET',
            headers: { 'Accept': 'application/json' },
            signal: AbortSignal.timeout(5000)
        });
        
        if (response.ok) {
            const data = await response.json();
            statusEl.textContent = '✅ Połączono';
            statusEl.style.color = '#28a745';
            AppState.isConnected = true;
            progressEl.style.width = '100%';
            updateConnectionStatus(true);
            return true;
        }
    } catch (error) {
        logMessage("warn", 'Brak połączenia z APIARY_COLLECTOR:', error.message);
    }
    
    // Brak połączenia nie uruchamia automatycznie danych demonstracyjnych.
    statusEl.textContent = '⚠️ Brak połączenia z kolektorem';
    statusEl.style.color = '#fd7e14';
    AppState.isConnected = false;
    progressEl.style.width = '30%';
    updateConnectionStatus(false);
    return false;
}

function updateConnectionStatus(connected) {
    const indicator = document.getElementById('connectionStatus');
    const statusText = document.getElementById('statusText');
    
    if (!indicator || !statusText) return; // Guard clause
    
    indicator.classList.toggle('connected', connected);
    indicator.classList.toggle('disconnected', !connected);
    if (connected) {
        indicator.style.backgroundColor = '';
        statusText.textContent = 'Połączono z APIARY_COLLECTOR';
    } else {
        indicator.style.backgroundColor = '';
        statusText.textContent = 'Brak połączenia — dane niedostępne';
    }
}

// ============================================================================
// POBIERANIE DANYCH
// ============================================================================
async function fetchHiveData() {
    try {
        const response = await fetch(`${CONFIG.apiBaseUrl}${CONFIG.endpoints.hives}`, {
            method: 'GET',
            headers: { 'Accept': 'application/json' },
            signal: AbortSignal.timeout(5000)
        });
        if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);

        const payload = await response.json();
        let source;
        if (Array.isArray(payload)) {
            source = payload;
        } else if (payload && typeof payload === 'object' &&
                   payload.hives && typeof payload.hives === 'object' && !Array.isArray(payload.hives)) {
            source = payload.hives;
        } else {
            throw new Error('Nieprawidłowa odpowiedź kolektora: brak mapy hives');
        }

        const toFiniteNumber = value => {
            if (value === null || value === undefined || value === '') return null;
            const number = Number(value);
            return Number.isFinite(number) ? number : null;
        };

        if (Array.isArray(source)) {
            AppState.hives = source;
        } else {
            AppState.hives = Object.entries(source).map(([id, hive]) => {
                const hasPerHiveTimestamp = hive.last_seen !== null && hive.last_seen !== undefined;
                const lastSeen = Number(hasPerHiveTimestamp ? hive.last_seen : (payload.timestamp || 0));
                const ageSeconds = Date.now() / 1000 - lastSeen;
                const online = hive.online === true && hasPerHiveTimestamp &&
                    lastSeen > 0 && ageSeconds >= 0 && ageSeconds <= 60;
                return {
                    hive_id: id,
                    temperature: toFiniteNumber(hive.temp),
                    humidity: toFiniteNumber(hive.hum),
                    weight: toFiniteNumber(hive.weight),
                    battery_level: toFiniteNumber(hive.bat),
                    // SGP41 nie mierzy CO2/eCO2; nie prezentuj tej wartości jako pomiaru.
                    co2_eq: null,
                    co2_available: false,
                    voc_idx: toFiniteNumber(hive.voc),
                    motion_detected: hive.motion ?? null,
                    is_online: online,
                    audio_dominant_freq: toFiniteNumber(hive.audio?.freq),
                    swarm_probability: toFiniteNumber(hive.audio?.swarm_prob),
                    timestamp: lastSeen
                };
            });
        }
        AppState.isConnected = true;
        AppState.lastRefresh = new Date();
        updateConnectionStatus(true);
        return AppState.hives;
    } catch (error) {
        logMessage("error", 'Błąd pobierania danych:', error.message);
        AppState.isConnected = false;
        AppState.hives = [];
        updateConnectionStatus(false);
        return AppState.hives;
    }
}

// ============================================================================
// RENDEROWANIE DASHBOARDU
// ============================================================================
function renderDashboard() {
    const hiveCardsContainer = document.getElementById('hiveCards');
    
    if (!hiveCardsContainer) {
        logMessage("warn", 'hiveCardsContainer element not found');
        return;
    }
    
    if (!AppState.hives || AppState.hives.length === 0) {
        hiveCardsContainer.innerHTML = `
            <div class="card" style="grid-column: 1/-1; text-align: center; padding: 3rem;">
                <h3 style="color: #666;">Brak danych z uli</h3>
                <p style="color: #999; margin-top: 1rem;">
                    Uruchom APIARY_COLLECTOR lub sprawdź połączenie sieciowe
                </p>
            </div>
        `;
        updateSummaryMetrics();
        return;
    }
    
    hiveCardsContainer.innerHTML = AppState.hives.map(hive => {
        const statusClass = hive.is_online ? 'status-online' : 'status-offline';
        const statusText = hive.is_online ? 'ONLINE' : 'OFFLINE';
        const safeHiveId = escapeHtml(hive.hive_id || 'UL-' + hive.id);
        
        return `
            <div class="card">
                <div class="card-header">
                    <h3 class="card-title">🐝 ${safeHiveId}</h3>
                    <span class="card-status ${statusClass}">${statusText}</span>
                </div>
                
                <div class="metric-grid">
                    <div class="metric-item">
                        <div class="metric-value">${hive.temperature?.toFixed(1) ?? '--'}</div>
                        <div class="metric-label">Temperatura</div>
                        <div class="metric-unit">°C</div>
                    </div>
                    <div class="metric-item">
                        <div class="metric-value">${hive.humidity?.toFixed(1) ?? '--'}</div>
                        <div class="metric-label">Wilgotność</div>
                        <div class="metric-unit">%RH</div>
                    </div>
                    <div class="metric-item">
                        <div class="metric-value">${hive.weight?.toFixed(3) ?? '--'}</div>
                        <div class="metric-label">Waga</div>
                        <div class="metric-unit">kg</div>
                    </div>
                    <div class="metric-item">
                        <div class="metric-value">${hive.battery_level ?? '--'}</div>
                        <div class="metric-label">Bateria</div>
                        <div class="metric-unit">%</div>
                    </div>
                </div>
                
                <div style="margin-top: 1rem; padding-top: 1rem; border-top: 1px solid var(--border-color);">
                    <div style="display: flex; gap: 0.5rem; flex-wrap: wrap;">
                        <span class="sensor-badge badge-co2">CO₂: ${hive.co2_eq || '--'} ppm</span>
                        <span class="sensor-badge badge-audio">VOC: ${hive.voc_idx || '--'}</span>
                        <span class="sensor-badge badge-radar">Ruch: ${hive.motion_detected ? 'TAK' : 'NIE'}</span>
                    </div>
                </div>
                
                ${hive.audio_dominant_freq ? `
                <div style="margin-top: 0.5rem; font-size: 0.85rem; color: #666;">
                    <strong>Audio:</strong> ${hive.audio_dominant_freq.toFixed(0)} Hz | 
                    Rojenie: ${(hive.swarm_probability * 100).toFixed(1)}%
                </div>
                ` : ''}
                
                <div style="margin-top: 1rem; display: flex; gap: 0.5rem;">
                    <button class="btn btn-primary" onclick="viewHiveDetails('${safeHiveId}')" style="flex: 1;">
                        📊 Szczegóły
                    </button>
                    <button class="btn btn-secondary" onclick="toggleEffector('${safeHiveId}')" style="flex: 1;">
                        ⚙️ Efektory
                    </button>
                </div>
            </div>
        `;
    }).join('');
    
    updateSummaryMetrics();
}

function updateSummaryMetrics() {
    const hives = AppState.hives || [];
    
    // Guard clauses for DOM elements
    const totalHivesEl = document.getElementById('totalHives');
    const onlineHivesEl = document.getElementById('onlineHives');
    const avgTempEl = document.getElementById('avgTemp');
    const avgHumidityEl = document.getElementById('avgHumidity');
    const totalWeightEl = document.getElementById('totalWeight');
    const alertsCountEl = document.getElementById('alertsCount');
    
    if (!totalHivesEl || !onlineHivesEl || !avgTempEl || !avgHumidityEl || !totalWeightEl || !alertsCountEl) {
        logMessage("warn", 'Some summary metrics DOM elements not found');
        return;
    }
    
    totalHivesEl.textContent = hives.length;
    onlineHivesEl.textContent = hives.filter(h => h.is_online).length;
    
    const avgTemp = hives.reduce((sum, h) => sum + (h.temperature || 0), 0) / (hives.length || 1);
    const avgHum = hives.reduce((sum, h) => sum + (h.humidity || 0), 0) / (hives.length || 1);
    const totalWeight = hives.reduce((sum, h) => sum + (h.weight || 0), 0);
    
    avgTempEl.textContent = avgTemp ? avgTemp.toFixed(1) : '--';
    avgHumidityEl.textContent = avgHum ? avgHum.toFixed(1) : '--';
    totalWeightEl.textContent = totalWeight ? totalWeight.toFixed(2) : '--';
    
    // Liczenie alertów (temperatura > próg)
    const alerts = hives.filter(h => (h.temperature || 0) > CONFIG.settings.alertThreshold).length;
    alertsCountEl.textContent = alerts;
    alertsCountEl.style.color = alerts > 0 ? '#dc3545' : 'var(--primary-color)';
}

// ============================================================================
// ZARZĄDZANIE SENSORAMI
// ============================================================================
function renderSensorsTable() {
    const tbody = document.getElementById('sensorsTable');
    if (!tbody) return;

    const sensors = [];
    AppState.hives.forEach(hive => {
        const lastRead = hive.timestamp ? new Date(hive.timestamp * 1000).toLocaleString('pl-PL') : 'Brak znacznika czasu';
        const status = hive.is_online ? 'active' : 'inactive';
        if (Number.isFinite(hive.temperature)) sensors.push({ id: `TEMP-${hive.hive_id}`, type: 'Temperatura', hive: hive.hive_id, value: `${hive.temperature.toFixed(1)} °C`, status, lastRead });
        if (Number.isFinite(hive.humidity)) sensors.push({ id: `HUM-${hive.hive_id}`, type: 'Wilgotność', hive: hive.hive_id, value: `${hive.humidity.toFixed(1)} %RH`, status, lastRead });
        if (Number.isFinite(hive.weight)) sensors.push({ id: `WGT-${hive.hive_id}`, type: 'Waga', hive: hive.hive_id, value: `${hive.weight.toFixed(3)} kg`, status, lastRead });
        if (Number.isFinite(hive.battery_level)) sensors.push({ id: `BAT-${hive.hive_id}`, type: 'Bateria', hive: hive.hive_id, value: `${hive.battery_level} %`, status, lastRead });
    });

    if (sensors.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" style="text-align:center;color:#666;">Brak rzeczywistych odczytów sensorów</td></tr>';
        return;
    }

    tbody.innerHTML = sensors.map(sensor => `
        <tr>
            <td><strong>${escapeHtml(sensor.id)}</strong></td>
            <td>${escapeHtml(sensor.type)}</td>
            <td>${escapeHtml(sensor.hive)}</td>
            <td>${escapeHtml(sensor.value)}</td>
            <td><span class="card-status ${sensor.status === 'active' ? 'status-online' : 'status-offline'}">${sensor.status === 'active' ? 'ONLINE' : 'OFFLINE'}</span></td>
            <td>${escapeHtml(sensor.lastRead)}</td>
        </tr>
    `).join('');
}

function showAddSensorModal() {
    alert('Konfiguracja sensorów przez WebUI nie jest obecnie obsługiwana.');
}
function renderEffectorsTable() {
    const tbody = document.getElementById('effectorsTable');
    if (!tbody) return;
    tbody.innerHTML = '<tr><td colspan="6" style="text-align:center;color:#666;">Rzeczywisty stan efektorów nie jest jeszcze udostępniany przez API. Panel działa w trybie tylko do odczytu.</td></tr>';
}
function getEffectorName(type) {
    const names = {
        fan: '🌀 Wentylator',
        heater: '🔥 Grzałka',
        humidifier: '💧 Nawilżacz',
        feeder: '🍯 Podkarmiacz',
        light: '💡 Oświetlenie',
        alarm: '🚨 Alarm'
    };
    return names[type] || type;
}

function showAddEffectorModal() {
    alert('Konfiguracja efektorów przez WebUI jest wyłączona. Nie istnieje bezpieczny endpoint zapisu.');
}
function toggleEffectorState() {
    alert('Sterowanie efektorami jest wyłączone: kolektor nie udostępnia bezpiecznego endpointu zapisu.');
}

// ============================================================================
// DANE HISTORYCZNE
// ============================================================================
function loadHistoryData() {
    const canvas = document.getElementById('historyChart');
    if (!canvas) return;
    if (historyChart) {
        historyChart.destroy();
        historyChart = null;
    }
    canvas.style.display = 'none';
    const container = canvas.parentElement;
    let message = container.querySelector('.history-empty-state');
    if (!message) {
        message = document.createElement('p');
        message.className = 'history-empty-state';
        message.style.cssText = 'padding:2rem;text-align:center;color:#666;';
        container.appendChild(message);
    }
    message.textContent = 'Historia pomiarów nie jest obecnie udostępniana przez kolektor. Wykres pojawi się po wdrożeniu trwałego magazynu danych historycznych.';
}

function renderHistoryChart(labels, data, label) {
    const canvas = document.getElementById('historyChart');
    if (!canvas) {
        logMessage("error", "Canvas element not found for history chart");
        return;
    }
    
    let ctx;
    try {
        ctx = canvas.getContext('2d');
    } catch (error) {
        logMessage("error", "Failed to get 2D context from canvas", error.message);
        return;
    }
    
    if (historyChart) {
        try {
            historyChart.destroy();
        } catch (error) {
            logMessage("warn", "Error destroying previous chart instance", error.message);
        }
    }
    
    try {
        historyChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: label,
                    data: data,
                    borderColor: '#2c5f2d',
                    backgroundColor: 'rgba(44, 95, 45, 0.1)',
                    tension: 0.4,
                    fill: true
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    legend: {
                        display: true,
                        position: 'top'
                    }
                },
                scales: {
                    y: {
                        beginAtZero: false
                    }
                }
            }
        });
    } catch (error) {
        logMessage("error", "Failed to create Chart.js instance", error.message);
    }
}

// ============================================================================
// LOGI
// ============================================================================
function loadLogs() {
    const container = document.getElementById('logContainer');
    if (!container) return;
    container.textContent = 'Kolektor nie udostępnia obecnie endpointu dziennika. Nie są wyświetlane przykładowe ani fikcyjne wpisy.';
}
function clearLogs() {
    const container = document.getElementById('logContainer');
    if (container) container.textContent = 'Dziennik kolektora nie jest dostępny przez obecne API.';
}

// ============================================================================
// USTAWIENIA
// ============================================================================
function initSettings() {
    const form = document.getElementById('settingsForm');
    if (form) {
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            
            CONFIG.settings.refreshInterval = Math.min(300, Math.max(5, parseInt(document.getElementById('refreshInterval').value, 10) || 10));
            CONFIG.refreshInterval = CONFIG.settings.refreshInterval * 1000;
            CONFIG.settings.alertThreshold = parseFloat(document.getElementById('alertThreshold').value);
            
            // Zapisz konfigurację do backendu
            saveLocalPreferences();
            
            // Zrestartuj auto-refresh z nowym interwałem
            stopAutoRefresh();
            startAutoRefresh();
            
            alert('✅ Ustawienia zapisane!');
        });
    }
}

// ============================================================================
// LOKALNE PREFERENCJE PRZEGLĄDARKI
// ============================================================================
function saveLocalPreferences() {
    try {
        localStorage.setItem('apiaryguard.settings', JSON.stringify(CONFIG.settings));
        logMessage('info', 'Ustawienia zapisano lokalnie w tej przeglądarce.');
    } catch (error) {
        logMessage('error', 'Nie udało się zapisać ustawień lokalnych', error.message);
    }
}
function startAutoRefresh() {
    refreshTimer = setInterval(() => {
        try {
            refreshAllData();
        } catch (error) {
            logMessage("error", "Błąd podczas automatycznego odświeżania", error.message);
        }
    }, CONFIG.refreshInterval);
    
    startCountdown();
}

function stopAutoRefresh() {
    if (refreshTimer) clearInterval(refreshTimer);
    if (countdownInterval) clearInterval(countdownInterval);
}

function startCountdown() {
    let seconds = CONFIG.refreshInterval / 1000;
    
    countdownInterval = setInterval(() => {
        try {
            seconds--;
            const timerElement = document.getElementById('refreshTimer');
            if (timerElement) {
                timerElement.textContent = `Odświeżanie za: ${seconds}s`;
            }
            
            if (seconds <= 0) {
                seconds = CONFIG.refreshInterval / 1000;
            }
        } catch (error) {
            logMessage("error", "Błąd w odliczaniu", error.message);
        }
    }, 1000);
}

async function refreshAllData() {
    if (AppState.isRefreshing) return;
    AppState.isRefreshing = true;
    try {
        // Ponawiaj próbę także po utracie łączności — bez danych demonstracyjnych.
        await fetchHiveData();
        renderDashboard();
        renderSensorsTable();
        renderEffectorsTable();
        updateHiveSelects();
    } catch (error) {
        logMessage("error", "Błąd odświeżania danych", error.message);
    } finally {
        AppState.isRefreshing = false;
        AppState.lastRefresh = new Date();
    }
}
// ============================================================================
// DEMO DANE (gdy brak połączenia z kolektorem)
// ============================================================================
function loadDemoData() {
    // Brak danych rzeczywistych oznacza pusty stan, nigdy fikcyjne pomiary.
    AppState.hives = [];
    AppState.sensors = [];
    AppState.effectors = [];
    renderDashboard();
    renderSensorsTable();
    renderEffectorsTable();
    updateHiveSelects();
}

// ============================================================================
// MODALS
// ============================================================================
function closeModal(modalId) {
    document.getElementById(modalId).classList.remove('active');
}

// Zamknij modal po kliknięciu poza zawartością - PRZENIESIONE DO DOMContentLoaded
// Ta funkcja jest wywoływana w initModals() wewnątrz DOMContentLoaded

// ============================================================================
// INICJALIZACJA MODALI
// ============================================================================
function initModals() {
    // Zamknij modal po kliknięciu poza zawartością
    document.querySelectorAll('.modal').forEach(modal => {
        modal.addEventListener('click', function(e) {
            if (e.target === this) {
                this.classList.remove('active');
            }
        });
    });
    
    // Inicjalizacja formularzy
    initSensorForm();
    initEffectorForm();
}

function initSensorForm() {
    const form = document.getElementById('addSensorForm');
    if (form) {
        form.addEventListener('submit', event => {
            event.preventDefault();
            alert('Dodawanie sensorów jest niedostępne. Konfigurację sprzętu należy wykonać w systemie urządzenia.');
        });
    }
}
function initEffectorForm() {
    const form = document.getElementById('addEffectorForm');
    if (form) {
        form.addEventListener('submit', event => {
            event.preventDefault();
            alert('Dodawanie efektorów jest niedostępne. Sterowanie pozostaje wyłączone do czasu wdrożenia bezpiecznego API.');
        });
    }
}
function viewHiveDetails(hiveId) {
    const hive = AppState.hives.find(h => h.hive_id === hiveId);
    if (!hive) {
        alert(`Nie znaleziono ula ${hiveId}`);
        return;
    }
    
    const modal = document.getElementById('hiveDetailsModal');
    if (modal) {
        const modalContent = modal.querySelector('.modal-content');
        if (modalContent) {
            modalContent.innerHTML = `
                <div class="modal-header">
                    <h2>🐝 Szczegóły ula ${escapeHtml(hiveId)}</h2>
                    <button class="modal-close" onclick="closeModal('hiveDetailsModal')">&times;</button>
                </div>
                <div class="metric-grid" style="margin-bottom: 1rem;">
                    <div class="metric-item">
                        <div class="metric-value">${hive.temperature?.toFixed(1) || '--'}</div>
                        <div class="metric-label">Temperatura</div>
                        <div class="metric-unit">°C</div>
                    </div>
                    <div class="metric-item">
                        <div class="metric-value">${hive.humidity?.toFixed(1) || '--'}</div>
                        <div class="metric-label">Wilgotność</div>
                        <div class="metric-unit">%RH</div>
                    </div>
                    <div class="metric-item">
                        <div class="metric-value">${hive.weight?.toFixed(3) || '--'}</div>
                        <div class="metric-label">Waga</div>
                        <div class="metric-unit">kg</div>
                    </div>
                    <div class="metric-item">
                        <div class="metric-value">${hive.battery_level || '--'}</div>
                        <div class="metric-label">Bateria</div>
                        <div class="metric-unit">%</div>
                    </div>
                </div>
                <div class="form-group">
                    <label>Status:</label>
                    <span class="card-status ${hive.is_online ? 'status-online' : 'status-offline'}">
                        ${hive.is_online ? 'ONLINE' : 'OFFLINE'}
                    </span>
                </div>
                <div class="form-group">
                    <label>Ostatni odczyt:</label>
                    <p>${hive.timestamp ? new Date(hive.timestamp * 1000).toLocaleString('pl-PL') : 'Brak danych'}</p>
                </div>
                <div class="form-group">
                    <label>Sensory:</label>
                    <div>
                        <span class="sensor-badge badge-temp">Temperatura</span>
                        <span class="sensor-badge badge-hum">Wilgotność</span>
                        <span class="sensor-badge badge-weight">Waga</span>
                        <span class="sensor-badge badge-bat">Bateria</span>
                        ${hive.co2_available === true ? '<span class="sensor-badge badge-co2">CO₂</span>' : ''}
                        ${hive.audio_dominant_freq !== undefined ? '<span class="sensor-badge badge-audio">Audio</span>' : ''}
                    </div>
                </div>
                <button class="btn btn-primary" onclick="closeModal('hiveDetailsModal')" style="width: 100%;">Zamknij</button>
            `;
        }
        modal.classList.add('active');
    } else {
        // Fallback - alert jeśli modal nie istnieje
        alert(`Szczegóły ula ${hiveId}: Temp=${hive.temperature}°C, Wilgotność=${hive.humidity}%RH, Waga=${hive.weight}kg`);
    }
}

function toggleEffector(hiveId) {
    alert(`Sterowanie efektorami ula ${hiveId} nie jest dostępne przez HTTP. Panel prezentuje wyłącznie dane odczytowe.`);
}
function editSensor() {
    alert('Edycja sensorów przez WebUI jest niedostępna — panel działa w trybie tylko do odczytu.');
}
function deleteSensor() {
    alert('Usuwanie sensorów przez WebUI jest niedostępne — panel działa w trybie tylko do odczytu.');
}
function editEffector() {
    alert('Edycja efektorów przez WebUI jest niedostępna — sterowanie pozostaje wyłączone.');
}
function exportData() {
    const dataStr = JSON.stringify(AppState.hives, null, 2);
    const blob = new Blob([dataStr], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `apiary_data_${new Date().toISOString().split('T')[0]}.json`;
    a.click();
    URL.revokeObjectURL(url);
}

// Form submissions są teraz obsługiwane w initSensorForm() i initEffectorForm()
// wywoływanych wewnątrz DOMContentLoaded przez initModals()

