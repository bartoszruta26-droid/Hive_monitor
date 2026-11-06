/**
 * ApiaryGuard WebUI - JavaScript Application
 * Komunikacja z APIARY_COLLECTOR przez HTTP API JSON
 * 
 * Endpointy API:
 * - GET /api/status - Status systemu
 * - GET /api/hives - Lista uli z danymi sensorów
 * - GET /health - Health check
 * - POST /api/sensors - Dodaj sensor
 * - POST /api/effectors - Dodaj effektor
 * - GET /api/history - Dane historyczne
 */

// ============================================================================
// KONFIGURACJA
// ============================================================================
const CONFIG = {
    apiBaseUrl: window.location.hostname ? `${window.location.protocol}//${window.location.hostname}:8080` : '',
    refreshInterval: 10000, // 10 sekund
    maxRetries: 3,
    endpoints: {
        status: '/api/status',
        hives: '/api/hives',
        health: '/health',
        sensors: '/api/sensors',
        effectors: '/api/effectors',
        history: '/api/history',
        logs: '/api/logs',
        config: '/api/config'
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
    loadConfigFromBackend(); // Najpierw załaduj konfigurację
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
async function loadConfigFromBackend() {
    try {
        const response = await fetch(`${CONFIG.apiBaseUrl}${CONFIG.endpoints.config}`, {
            method: 'GET',
            headers: { 'Accept': 'application/json' },
            timeout: 5000
        });
        
        if (response.ok) {
            const data = await response.json();
            if (data.config) {
                // Scal konfigurację z backendu z domyślnymi ustawieniami
                if (data.config.preferences) {
                    const prefs = data.config.preferences;
                    CONFIG.settings.language = prefs.language || CONFIG.settings.language;
                    CONFIG.settings.temperatureUnit = prefs.temperature_unit || CONFIG.settings.temperatureUnit;
                    CONFIG.settings.humidityUnit = prefs.humidity_unit || CONFIG.settings.humidityUnit;
                    CONFIG.settings.weightUnit = prefs.weight_unit || CONFIG.settings.weightUnit;
                    CONFIG.settings.dateFormat = prefs.date_format || CONFIG.settings.dateFormat;
                    CONFIG.settings.timeFormat = prefs.time_format || CONFIG.settings.timeFormat;
                    CONFIG.settings.theme = prefs.theme || CONFIG.settings.theme;
                    CONFIG.settings.showCharts = prefs.show_charts !== undefined ? prefs.show_charts : CONFIG.settings.showCharts;
                }
                
                if (data.config.notifications) {
                    const notif = data.config.notifications;
                    CONFIG.settings.enableNotifications = notif.enable_notifications !== undefined ? notif.enable_notifications : CONFIG.settings.enableNotifications;
                    CONFIG.settings.emailNotifications = notif.email_notifications !== undefined ? notif.email_notifications : CONFIG.settings.emailNotifications;
                    CONFIG.settings.pushNotifications = notif.push_notifications !== undefined ? notif.push_notifications : CONFIG.settings.pushNotifications;
                    CONFIG.settings.tempMinAlert = notif.temp_min_alert || CONFIG.settings.tempMinAlert;
                    CONFIG.settings.tempMaxAlert = notif.temp_max_alert || CONFIG.settings.tempMaxAlert;
                    CONFIG.settings.humidityMinAlert = notif.humidity_min_alert || CONFIG.settings.humidityMinAlert;
                    CONFIG.settings.humidityMaxAlert = notif.humidity_max_alert || CONFIG.settings.humidityMaxAlert;
                }
                
                if (data.config.intervals) {
                    const intv = data.config.intervals;
                    CONFIG.settings.refreshInterval = intv.sensor_update_interval || CONFIG.settings.refreshInterval;
                }
                
                logMessage('info', 'Konfiguracja załadowana z backendu', data.config);
            }
        }
    } catch (error) {
        logMessage('warn', 'Nie udało się załadować konfiguracji z backendu, używam domyślnych', error.message);
    }
}

// ============================================================================
// ZARZĄDZANIE ZAKŁADKAMI
// ============================================================================
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
    
    // Fallback do demo danych
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
        // Kolektor zwraca { timestamp, hive_count, hives: { "UL-1": { temp, hum, ... } } }.
        const source = payload && payload.hives ? payload.hives : payload;
        if (Array.isArray(source)) {
            AppState.hives = source;
        } else if (source && typeof source === 'object') {
            AppState.hives = Object.entries(source).map(([id, hive]) => {
                const lastSeen = Number(hive.last_seen || payload.timestamp || 0);
                const online = hive.online === true && lastSeen > 0 &&
                    (Date.now() / 1000 - lastSeen) <= 60;
                return {
                    hive_id: id,
                    temperature: Number.isFinite(Number(hive.temp)) ? Number(hive.temp) : null,
                    humidity: Number.isFinite(Number(hive.hum)) ? Number(hive.hum) : null,
                    weight: Number.isFinite(Number(hive.weight)) ? Number(hive.weight) : null,
                    battery_level: Number.isFinite(Number(hive.bat)) ? Number(hive.bat) : null,
                    // SGP41 nie mierzy CO2/eCO2; nie prezentuj tej wartości jako pomiaru.
                    co2_eq: null,
                    co2_available: false,
                    voc_idx: hive.voc ?? null,
                    motion_detected: hive.motion ?? null,
                    is_online: online,
                    audio_dominant_freq: hive.audio?.freq ?? null,
                    swarm_probability: hive.audio?.swarm_prob ?? null,
                    timestamp: lastSeen
                };
            });
        } else {
            throw new Error('Nieprawidłowy format odpowiedzi /api/hives');
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
    
    // Generowanie sensorów z danych uli
    const sensors = [];
    AppState.hives.forEach(hive => {
        if (hive.temperature !== undefined) {
            sensors.push({ id: `TEMP-${hive.hive_id}`, type: 'temperature', hive: hive.hive_id, status: hive.is_online ? 'active' : 'inactive', lastRead: new Date().toISOString() });
        }
        if (hive.humidity !== undefined) {
            sensors.push({ id: `HUM-${hive.hive_id}`, type: 'humidity', hive: hive.hive_id, status: hive.is_online ? 'active' : 'inactive', lastRead: new Date().toISOString() });
        }
        if (hive.weight !== undefined) {
            sensors.push({ id: `WGT-${hive.hive_id}`, type: 'weight', hive: hive.hive_id, status: hive.is_online ? 'active' : 'inactive', lastRead: new Date().toISOString() });
        }
    });
    
    if (sensors.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; color: #666;">Brak sensorów</td></tr>';
        return;
    }
    
    tbody.innerHTML = sensors.map(sensor => `
        <tr>
            <td><strong>${escapeHtml(sensor.id)}</strong></td>
            <td><span class="sensor-badge badge-${escapeHtml(sensor.type)}">${escapeHtml(sensor.type)}</span></td>
            <td>${escapeHtml(sensor.hive)}</td>
            <td>
                <span class="card-status ${sensor.status === 'active' ? 'status-online' : 'status-offline'}">
                    ${sensor.status === 'active' ? 'Aktywny' : 'Nieaktywny'}
                </span>
            </td>
            <td>${new Date(sensor.lastRead).toLocaleString('pl-PL')}</td>
            <td>
                <span class="metric-label">Tylko odczyt</span>
            </td>
        </tr>
    `).join('');
}

function showAddSensorModal() {
    updateHiveSelects();
    document.getElementById('addSensorModal').classList.add('active');
}

function updateHiveSelects() {
    const selects = ['sensorHive', 'effectorHive', 'historyHiveSelect'];
    
    selects.forEach(selectId => {
        const select = document.getElementById(selectId);
        if (!select) return;
        
        const currentValue = select.value;
        const options = (AppState.hives || []).map(h => `<option value="${escapeHtml(h.hive_id)}">${escapeHtml(h.hive_id)}</option>`).join('');
        
        if (selectId === 'historyHiveSelect') {
            select.innerHTML = '<option value="all">Wszystkie ule</option>' + options;
        } else {
            select.innerHTML = options;
        }
        
        if (currentValue) select.value = currentValue;
    });
}

// ============================================================================
// ZARZĄDZANIE EFEKTORAMI
// ============================================================================
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
    updateHiveSelects();
    document.getElementById('addEffectorModal').classList.add('active');
}

function toggleEffectorState() {
    alert('Sterowanie efektorami jest wyłączone: kolektor nie udostępnia bezpiecznego endpointu zapisu.');
}

// ============================================================================
// DANE HISTORYCZNE
// ============================================================================
async function loadHistoryData() {
    const canvas = document.getElementById('historyChart');
    if (!canvas) return;
    const container = canvas.parentElement;
    if (historyChart) {
        historyChart.destroy();
        historyChart = null;
    }
    canvas.style.display = 'none';
    let message = container.querySelector('.history-empty-state');
    if (!message) {
        message = document.createElement('p');
        message.className = 'history-empty-state';
        message.style.cssText = 'padding:2rem;text-align:center;color:#666;';
        container.appendChild(message);
    }
    message.textContent = 'Ładowanie danych historycznych…';
    try {
        const response = await fetch(`${CONFIG.apiBaseUrl}${CONFIG.endpoints.history}`, {
            headers: { 'Accept': 'application/json' },
            signal: AbortSignal.timeout(5000)
        });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const payload = await response.json();
        if (Array.isArray(payload.labels) && Array.isArray(payload.data) && payload.labels.length && payload.data.length) {
            message.remove();
            canvas.style.display = 'block';
            renderHistoryChart(payload.labels, payload.data, document.getElementById('historyMetricSelect')?.value || 'Pomiar');
        } else {
            message.textContent = 'Kolektor nie udostępnia jeszcze rzeczywistych danych historycznych. Wykres pozostaje pusty zamiast generować dane symulowane.';
        }
    } catch (error) {
        message.textContent = 'Nie udało się pobrać historii. Sprawdź połączenie z kolektorem.';
        logMessage('warn', 'Błąd pobierania historii', error.message);
    }
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
    
    // Przykładowe logi
    const logs = [
        { level: 'info', message: 'System uruchomiony', timestamp: new Date() },
        { level: 'info', message: 'Połączono z UL-001', timestamp: new Date(Date.now() - 60000) },
        { level: 'warning', message: 'Wysoka temperatura w UL-002: 38°C', timestamp: new Date(Date.now() - 120000) },
        { level: 'info', message: 'Zapisano dane pomiarowe', timestamp: new Date(Date.now() - 180000) },
        { level: 'error', message: 'Utracono połączenie z UL-003', timestamp: new Date(Date.now() - 240000) },
        { level: 'debug', message: 'Thread sensor_001 started', timestamp: new Date(Date.now() - 300000) }
    ];
    
    container.innerHTML = logs.map(log => `
        <div class="log-entry log-${log.level}">
            [${log.timestamp.toLocaleString('pl-PL')}] [${log.level.toUpperCase()}] ${log.message}
        </div>
    `).join('');
}

function clearLogs() {
    document.getElementById('logContainer').innerHTML = '';
}

// ============================================================================
// USTAWIENIA
// ============================================================================
function initSettings() {
    const form = document.getElementById('settingsForm');
    if (form) {
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            
            CONFIG.settings.refreshInterval = parseInt(document.getElementById('refreshInterval').value);
            CONFIG.settings.alertThreshold = parseFloat(document.getElementById('alertThreshold').value);
            
            // Zapisz konfigurację do backendu
            saveConfigToBackend();
            
            // Zrestartuj auto-refresh z nowym interwałem
            stopAutoRefresh();
            startAutoRefresh();
            
            alert('✅ Ustawienia zapisane!');
        });
    }
}

// ============================================================================
// ZAPIS KONFIGURACJI DO BACKENDU
// ============================================================================
async function saveConfigToBackend() {
    try {
        const configData = {
            preferences: {
                language: CONFIG.settings.language,
                temperature_unit: CONFIG.settings.temperatureUnit,
                humidity_unit: CONFIG.settings.humidityUnit,
                weight_unit: CONFIG.settings.weightUnit,
                date_format: CONFIG.settings.dateFormat,
                time_format: CONFIG.settings.timeFormat,
                theme: CONFIG.settings.theme,
                show_charts: CONFIG.settings.showCharts
            },
            notifications: {
                enable_notifications: CONFIG.settings.enableNotifications,
                email_notifications: CONFIG.settings.emailNotifications,
                push_notifications: CONFIG.settings.pushNotifications,
                temp_min_alert: CONFIG.settings.tempMinAlert,
                temp_max_alert: CONFIG.settings.tempMaxAlert,
                humidity_min_alert: CONFIG.settings.humidityMinAlert,
                humidity_max_alert: CONFIG.settings.humidityMaxAlert
            },
            intervals: {
                sensor_update_interval: CONFIG.settings.refreshInterval,
                screen_update_interval: 5,
                log_refresh_interval: 10,
                data_sync_interval: 300,
                backup_interval: 86400
            }
        };
        
        const response = await fetch(`${CONFIG.apiBaseUrl}${CONFIG.endpoints.config}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ config: configData })
        });
        
        if (response.ok) {
            logMessage("info", '✅ Konfiguracja zapisana do backendu');
        } else {
            logMessage("error", '❌ Błąd zapisu konfiguracji');
        }
    } catch (error) {
        logMessage("error", '⚠️ Nie udało się zapisać konfiguracji:', error.message);
    }
}

// ============================================================================
// AUTO-REFRESH
// ============================================================================
let refreshTimer = null;
let countdownInterval = null;

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
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            
            // Walidacja client-side
            const sensorType = document.getElementById('sensorType').value;
            const sensorHive = document.getElementById('sensorHive').value;
            
            if (!sensorType || !sensorHive) {
                logMessage('error', 'Wszystkie pola są wymagane');
                alert('❌ Wszystkie pola są wymagane');
                return;
            }
            
            // Sanityzacja danych przed wysłaniem
            const safeType = escapeHtml(sensorType);
            const safeHive = escapeHtml(sensorHive);
            
            logMessage('info', `Dodawanie sensora: typ=${safeType}, ul=${safeHive}`);
            alert(`✅ Dodano sensor ${safeType} dla ula ${safeHive} (symulacja)`);
            closeModal('addSensorModal');
        });
    }
}

function initEffectorForm() {
    const form = document.getElementById('addEffectorForm');
    if (form) {
        form.addEventListener('submit', function(e) {
            e.preventDefault();
            
            // Walidacja client-side
            const effectorType = document.getElementById('effectorType').value;
            const effectorHive = document.getElementById('effectorHive').value;
            
            if (!effectorType || !effectorHive) {
                logMessage('error', 'Wszystkie pola są wymagane');
                alert('❌ Wszystkie pola są wymagane');
                return;
            }
            
            // Sanityzacja danych przed wysłaniem
            const safeType = escapeHtml(effectorType);
            const safeHive = escapeHtml(effectorHive);
            
            logMessage('info', `Dodawanie efektora: typ=${safeType}, ul=${safeHive}`);
            alert(`✅ Dodano effektor ${safeType} dla ula ${safeHive} (symulacja)`);
            closeModal('addEffectorModal');
        });
    }
}

// ============================================================================
// AKCJE - PEŁNE IMPLEMENTACJE
// ============================================================================
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
                    <h2>🐝 Szczegóły ula ${hiveId}</h2>
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
                    <p>${hive.last_update ? new Date(hive.last_update).toLocaleString('pl-PL') : 'Brak danych'}</p>
                </div>
                <div class="form-group">
                    <label>Sensory:</label>
                    <div>
                        <span class="sensor-badge badge-temp">Temperatura</span>
                        <span class="sensor-badge badge-hum">Wilgotność</span>
                        <span class="sensor-badge badge-weight">Waga</span>
                        <span class="sensor-badge badge-bat">Bateria</span>
                        ${hive.co2_eq !== undefined ? '<span class="sensor-badge badge-co2">CO₂</span>' : ''}
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
function editSensor(sensorId) {
    const sensor = AppState.sensors.find(s => s.id === sensorId) || 
                   AppState.hives.flatMap(h => [
                       { id: `TEMP-${h.hive_id}`, type: 'temperature', hive: h.hive_id },
                       { id: `HUM-${h.hive_id}`, type: 'humidity', hive: h.hive_id }
                   ]).find(s => s.id === sensorId);
    
    if (!sensor) {
        alert(`Nie znaleziono sensora ${sensorId}`);
        return;
    }
    
    const modal = document.getElementById('editSensorModal');
    if (modal) {
        const modalContent = modal.querySelector('.modal-content');
        if (modalContent) {
            modalContent.innerHTML = `
                <div class="modal-header">
                    <h2>✏️ Edycja sensora ${sensorId}</h2>
                    <button class="modal-close" onclick="closeModal('editSensorModal')">&times;</button>
                </div>
                <form id="editSensorForm">
                    <div class="form-group">
                        <label for="editSensorType">Typ:</label>
                        <input type="text" id="editSensorType" class="form-control" value="${sensor.type}" readonly>
                    </div>
                    <div class="form-group">
                        <label for="editSensorHive">Ul:</label>
                        <select id="editSensorHive" class="form-control">
                            ${AppState.hives.map(h => `<option value="${h.hive_id}" ${h.hive_id === sensor.hive ? 'selected' : ''}>${h.hive_id}</option>`).join('')}
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="editSensorStatus">Status:</label>
                        <select id="editSensorStatus" class="form-control">
                            <option value="active" ${sensor.status === 'active' ? 'selected' : ''}>Aktywny</option>
                            <option value="inactive" ${sensor.status === 'inactive' ? 'selected' : ''}>Nieaktywny</option>
                        </select>
                    </div>
                    <div style="display: flex; gap: 0.5rem; margin-top: 1rem;">
                        <button type="submit" class="btn btn-primary" style="flex: 1;">💾 Zapisz</button>
                        <button type="button" class="btn btn-secondary" onclick="closeModal('editSensorModal')" style="flex: 1;">Anuluj</button>
                    </div>
                </form>
            `;
            
            document.getElementById('editSensorForm').addEventListener('submit', function(e) {
                e.preventDefault();
                const newHive = document.getElementById('editSensorHive').value;
                const newStatus = document.getElementById('editSensorStatus').value;
                
                // TODO: Wysyłanie do API
                logMessage("info", `Updating sensor ${sensorId}: hive=${newHive}, status=${newStatus}`);
                alert(`✅ Zapisano zmiany dla sensora ${sensorId}`);
                closeModal('editSensorModal');
                renderSensorsTable();
            });
        }
        modal.classList.add('active');
    } else {
        alert(`Edycja sensora ${sensorId} - dostępna po dodaniu modala editSensorModal`);
    }
}

function deleteSensor(sensorId) {
    if (confirm(`Czy na pewno chcesz usunąć sensor ${sensorId}? Ta operacja jest nieodwracalna.`)) {
        // TODO: Wysyłanie DELETE request do API
        logMessage("info", `Deleting sensor ${sensorId}`);
        
        // Symulacja usunięcia
        const index = AppState.sensors.findIndex(s => s.id === sensorId);
        if (index > -1) {
            AppState.sensors.splice(index, 1);
        }
        
        alert(`✅ Usunięto sensor ${sensorId}`);
        renderSensorsTable();
    }
}

function editEffector(effectorId) {
    const effector = AppState.effectors.find(e => e.id === effectorId) || 
                     { id: effectorId, type: 'unknown', hive: 'UL-1', status: 'active', state: 'off' };
    
    const modal = document.getElementById('editEffectorModal');
    if (modal) {
        const modalContent = modal.querySelector('.modal-content');
        if (modalContent) {
            modalContent.innerHTML = `
                <div class="modal-header">
                    <h2>✏️ Edycja efektora ${effectorId}</h2>
                    <button class="modal-close" onclick="closeModal('editEffectorModal')">&times;</button>
                </div>
                <form id="editEffectorForm">
                    <div class="form-group">
                        <label for="editEffectorType">Typ:</label>
                        <select id="editEffectorType" class="form-control">
                            <option value="fan" ${effector.type === 'fan' ? 'selected' : ''}>Wentylator</option>
                            <option value="heater" ${effector.type === 'heater' ? 'selected' : ''}>Grzałka</option>
                            <option value="humidifier" ${effector.type === 'humidifier' ? 'selected' : ''}>Nawilżacz</option>
                            <option value="feeder" ${effector.type === 'feeder' ? 'selected' : ''}>Podkarmiacz</option>
                            <option value="light" ${effector.type === 'light' ? 'selected' : ''}>Oświetlenie</option>
                            <option value="alarm" ${effector.type === 'alarm' ? 'selected' : ''}>Alarm</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="editEffectorHive">Ul:</label>
                        <select id="editEffectorHive" class="form-control">
                            ${AppState.hives.map(h => `<option value="${h.hive_id}" ${h.hive_id === effector.hive ? 'selected' : ''}>${h.hive_id}</option>`).join('')}
                        </select>
                    </div>
                    <div class="form-group">
                        <label for="editEffectorStatus">Status:</label>
                        <select id="editEffectorStatus" class="form-control">
                            <option value="active" ${effector.status === 'active' ? 'selected' : ''}>Aktywny</option>
                            <option value="inactive" ${effector.status === 'inactive' ? 'selected' : ''}>Nieaktywny</option>
                        </select>
                    </div>
                    <div style="display: flex; gap: 0.5rem; margin-top: 1rem;">
                        <button type="submit" class="btn btn-primary" style="flex: 1;">💾 Zapisz</button>
                        <button type="button" class="btn btn-secondary" onclick="closeModal('editEffectorModal')" style="flex: 1;">Anuluj</button>
                    </div>
                </form>
            `;
            
            document.getElementById('editEffectorForm').addEventListener('submit', function(e) {
                e.preventDefault();
                const newType = document.getElementById('editEffectorType').value;
                const newHive = document.getElementById('editEffectorHive').value;
                const newStatus = document.getElementById('editEffectorStatus').value;
                
                // TODO: Wysyłanie do API
                logMessage("info", `Updating effector ${effectorId}: type=${newType}, hive=${newHive}, status=${newStatus}`);
                alert(`✅ Zapisano zmiany dla efektora ${effectorId}`);
                closeModal('editEffectorModal');
                renderEffectorsTable();
            });
        }
        modal.classList.add('active');
    } else {
        alert(`Edycja efektora ${effectorId} - dostępna po dodaniu modala editEffectorModal`);
    }
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

