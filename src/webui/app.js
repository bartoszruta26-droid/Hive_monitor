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
    apiBaseUrl: window.location.hostname ? `http://${window.location.hostname}:8080` : '',
    refreshInterval: 10000, // 10 sekund
    endpoints: {
        status: '/api/status',
        hives: '/api/hives',
        health: '/health',
        sensors: '/api/sensors',
        effectors: '/api/effectors',
        history: '/api/history',
        logs: '/api/logs'
    }
};

// Stan aplikacji
let appState = {
    hives: [],
    sensors: [],
    effectors: [],
    logs: [],
    settings: {
        refreshInterval: 10,
        alertThreshold: 35
    },
    isConnected: false,
    lastRefresh: null
};

// Chart instance
let historyChart = null;

// ============================================================================
// INICJALIZACJA
// ============================================================================
document.addEventListener('DOMContentLoaded', function() {
    initTabs();
    initSettings();
    loadDemoData(); // Demo dane na początek
    startAutoRefresh();
    checkCollectorConnection();
});

// ============================================================================
// ZARZĄDZANIE ZAKŁADKAMI
// ============================================================================
function initTabs() {
    const tabs = document.querySelectorAll('.nav-tab');
    tabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const tabId = this.getAttribute('data-tab');
            
            // Usuń active ze wszystkich
            tabs.forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
            
            // Dodaj active do klikniętej
            this.classList.add('active');
            document.getElementById(tabId).classList.add('active');
            
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
    
    try {
        progressEl.style.width = '50%';
        
        const response = await fetch(`${CONFIG.apiBaseUrl}${CONFIG.endpoints.health}`, {
            method: 'GET',
            headers: { 'Accept': 'application/json' }
        });
        
        if (response.ok) {
            const data = await response.json();
            statusEl.textContent = '✅ Połączono';
            statusEl.style.color = '#28a745';
            appState.isConnected = true;
            progressEl.style.width = '100%';
            updateConnectionStatus(true);
            return true;
        }
    } catch (error) {
        console.log('Brak połączenia z APIARY_COLLECTOR, używam danych demo');
    }
    
    // Fallback do demo danych
    statusEl.textContent = '⚠️ Tryb demo (brak kolektora)';
    statusEl.style.color = '#fd7e14';
    appState.isConnected = false;
    progressEl.style.width = '30%';
    updateConnectionStatus(false);
    return false;
}

function updateConnectionStatus(connected) {
    const indicator = document.getElementById('connectionStatus');
    const statusText = document.getElementById('statusText');
    
    if (connected) {
        indicator.style.backgroundColor = '#28a745';
        statusText.textContent = 'Połączono z APIARY_COLLECTOR';
    } else {
        indicator.style.backgroundColor = '#fd7e14';
        statusText.textContent = 'Tryb demo';
    }
}

// ============================================================================
// POBIERANIE DANYCH
// ============================================================================
async function fetchHiveData() {
    try {
        const response = await fetch(`${CONFIG.apiBaseUrl}${CONFIG.endpoints.hives}`, {
            method: 'GET',
            headers: { 'Accept': 'application/json' }
        });
        
        if (response.ok) {
            const data = await response.json();
            appState.hives = data.hives || data;
            return appState.hives;
        }
    } catch (error) {
        console.error('Błąd pobierania danych:', error);
    }
    
    return null;
}

// ============================================================================
// RENDEROWANIE DASHBOARDU
// ============================================================================
function renderDashboard() {
    const hiveCardsContainer = document.getElementById('hiveCards');
    
    if (!appState.hives || appState.hives.length === 0) {
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
    
    hiveCardsContainer.innerHTML = appState.hives.map(hive => {
        const statusClass = hive.is_online ? 'status-online' : 'status-offline';
        const statusText = hive.is_online ? 'ONLINE' : 'OFFLINE';
        
        return `
            <div class="card">
                <div class="card-header">
                    <h3 class="card-title">🐝 ${hive.hive_id || 'UL-' + hive.id}</h3>
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
                    <button class="btn btn-primary" onclick="viewHiveDetails('${hive.hive_id}')" style="flex: 1;">
                        📊 Szczegóły
                    </button>
                    <button class="btn btn-secondary" onclick="toggleEffector('${hive.hive_id}')" style="flex: 1;">
                        ⚙️ Efektory
                    </button>
                </div>
            </div>
        `;
    }).join('');
    
    updateSummaryMetrics();
}

function updateSummaryMetrics() {
    const hives = appState.hives || [];
    
    document.getElementById('totalHives').textContent = hives.length;
    document.getElementById('onlineHives').textContent = hives.filter(h => h.is_online).length;
    
    const avgTemp = hives.reduce((sum, h) => sum + (h.temperature || 0), 0) / hives.length;
    const avgHum = hives.reduce((sum, h) => sum + (h.humidity || 0), 0) / hives.length;
    const totalWeight = hives.reduce((sum, h) => sum + (h.weight || 0), 0);
    
    document.getElementById('avgTemp').textContent = avgTemp ? avgTemp.toFixed(1) : '--';
    document.getElementById('avgHumidity').textContent = avgHum ? avgHum.toFixed(1) : '--';
    document.getElementById('totalWeight').textContent = totalWeight ? totalWeight.toFixed(2) : '--';
    
    // Liczenie alertów (temperatura > próg)
    const alerts = hives.filter(h => (h.temperature || 0) > CONFIG.settings.alertThreshold).length;
    document.getElementById('alertsCount').textContent = alerts;
    document.getElementById('alertsCount').style.color = alerts > 0 ? '#dc3545' : 'var(--primary-color)';
}

// ============================================================================
// ZARZĄDZANIE SENSORAMI
// ============================================================================
function renderSensorsTable() {
    const tbody = document.getElementById('sensorsTable');
    
    // Generowanie sensorów z danych uli
    const sensors = [];
    appState.hives.forEach(hive => {
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
            <td><strong>${sensor.id}</strong></td>
            <td><span class="sensor-badge badge-${sensor.type}">${sensor.type}</span></td>
            <td>${sensor.hive}</td>
            <td>
                <span class="card-status ${sensor.status === 'active' ? 'status-online' : 'status-offline'}">
                    ${sensor.status === 'active' ? 'Aktywny' : 'Nieaktywny'}
                </span>
            </td>
            <td>${new Date(sensor.lastRead).toLocaleString('pl-PL')}</td>
            <td>
                <button class="btn btn-secondary" onclick="editSensor('${sensor.id}')" style="padding: 0.3rem 0.6rem; font-size: 0.85rem;">✏️</button>
                <button class="btn btn-danger" onclick="deleteSensor('${sensor.id}')" style="padding: 0.3rem 0.6rem; font-size: 0.85rem;">🗑️</button>
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
        const options = appState.hives.map(h => `<option value="${h.hive_id}">${h.hive_id}</option>`).join('');
        
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
    
    // Przykładowe efektory
    const effectors = [];
    appState.hives.forEach(hive => {
        effectors.push({ id: `FAN-${hive.hive_id}`, type: 'fan', hive: hive.hive_id, status: 'active', state: 'off' });
        effectors.push({ id: `HEAT-${hive.hive_id}`, type: 'heater', hive: hive.hive_id, status: 'active', state: 'off' });
    });
    
    if (effectors.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; color: #666;">Brak efektorów</td></tr>';
        return;
    }
    
    tbody.innerHTML = effectors.map(eff => `
        <tr>
            <td><strong>${eff.id}</strong></td>
            <td>${getEffectorName(eff.type)}</td>
            <td>${eff.hive}</td>
            <td>
                <span class="card-status ${eff.status === 'active' ? 'status-online' : 'status-offline'}">
                    ${eff.status === 'active' ? 'Aktywny' : 'Nieaktywny'}
                </span>
            </td>
            <td>
                <span class="card-status ${eff.state === 'on' ? 'status-online' : 'status-warning'}">
                    ${eff.state === 'on' ? 'WŁ.' : 'WYŁ.'}
                </span>
            </td>
            <td>
                <button class="btn ${eff.state === 'on' ? 'btn-danger' : 'btn-success'}" 
                        onclick="toggleEffectorState('${eff.id}')" 
                        style="padding: 0.3rem 0.6rem; font-size: 0.85rem;">
                    ${eff.state === 'on' ? '🔴 WYŁ' : '🟢 WŁ'}
                </button>
                <button class="btn btn-secondary" onclick="editEffector('${eff.id}')" style="padding: 0.3rem 0.6rem; font-size: 0.85rem;">✏️</button>
            </td>
        </tr>
    `).join('');
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

function toggleEffectorState(effectorId) {
    alert(`Przełączanie efektora ${effectorId} (symulacja)`);
    // W produkcji: fetch POST do API
}

// ============================================================================
// DANE HISTORYCZNE
// ============================================================================
function loadHistoryData() {
    const hiveSelect = document.getElementById('historyHiveSelect').value;
    const metricSelect = document.getElementById('historyMetricSelect').value;
    
    // Generowanie przykładowych danych historycznych
    const labels = [];
    const data = [];
    const now = new Date();
    
    for (let i = 24; i >= 0; i--) {
        const time = new Date(now.getTime() - i * 3600000);
        labels.push(time.toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit' }));
        // Symulacja danych
        data.push(20 + Math.random() * 10 + Math.sin(i / 4) * 5);
    }
    
    renderHistoryChart(labels, data, metricSelect);
}

function renderHistoryChart(labels, data, label) {
    const ctx = document.getElementById('historyChart').getContext('2d');
    
    if (historyChart) {
        historyChart.destroy();
    }
    
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
    form.addEventListener('submit', function(e) {
        e.preventDefault();
        
        CONFIG.settings.refreshInterval = parseInt(document.getElementById('refreshInterval').value);
        CONFIG.settings.alertThreshold = parseFloat(document.getElementById('alertThreshold').value);
        
        // Zrestartuj auto-refresh z nowym interwałem
        stopAutoRefresh();
        startAutoRefresh();
        
        alert('✅ Ustawienia zapisane!');
    });
}

// ============================================================================
// AUTO-REFRESH
// ============================================================================
let refreshTimer = null;
let countdownInterval = null;

function startAutoRefresh() {
    refreshTimer = setInterval(() => {
        refreshAllData();
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
        seconds--;
        document.getElementById('refreshTimer').textContent = `Odświeżanie za: ${seconds}s`;
        
        if (seconds <= 0) {
            seconds = CONFIG.refreshInterval / 1000;
        }
    }, 1000);
}

function refreshAllData() {
    if (appState.isConnected) {
        fetchHiveData().then(data => {
            if (data) {
                renderDashboard();
                renderSensorsTable();
                renderEffectorsTable();
            }
        });
    }
    appState.lastRefresh = new Date();
}

// ============================================================================
// DEMO DANE (gdy brak połączenia z kolektorem)
// ============================================================================
function loadDemoData() {
    appState.hives = [
        {
            hive_id: 'UL-001',
            temperature: 24.5,
            humidity: 55.2,
            weight: 45.300,
            battery_level: 98,
            co2_eq: 450,
            voc_idx: 35,
            motion_detected: 1,
            is_online: true,
            audio_dominant_freq: 250,
            swarm_probability: 0.15
        },
        {
            hive_id: 'UL-002',
            temperature: 26.1,
            humidity: 58.7,
            weight: 48.750,
            battery_level: 95,
            co2_eq: 480,
            voc_idx: 42,
            motion_detected: 0,
            is_online: true,
            audio_dominant_freq: 280,
            swarm_probability: 0.22
        },
        {
            hive_id: 'UL-003',
            temperature: 23.8,
            humidity: 52.1,
            weight: 42.100,
            battery_level: 87,
            co2_eq: 520,
            voc_idx: 38,
            motion_detected: 1,
            is_online: false,
            audio_dominant_freq: 220,
            swarm_probability: 0.08
        }
    ];
    
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

// Zamknij modal po kliknięciu poza zawartością
document.querySelectorAll('.modal').forEach(modal => {
    modal.addEventListener('click', function(e) {
        if (e.target === this) {
            this.classList.remove('active');
        }
    });
});

// ============================================================================
// AKCJE
// ============================================================================
function viewHiveDetails(hiveId) {
    alert(`Szczegóły ula ${hiveId} (rozbudowa w wersji premium)`);
}

function toggleEffector(hiveId) {
    alert(`Panel efektorów dla ${hiveId}`);
}

function editSensor(sensorId) {
    alert(`Edycja sensora ${sensorId}`);
}

function deleteSensor(sensorId) {
    if (confirm(`Czy na pewno usunąć sensor ${sensorId}?`)) {
        alert(`Usunięto sensor ${sensorId}`);
    }
}

function editEffector(effectorId) {
    alert(`Edycja efektora ${effectorId}`);
}

function exportData() {
    const dataStr = JSON.stringify(appState.hives, null, 2);
    const blob = new Blob([dataStr], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `apiary_data_${new Date().toISOString().split('T')[0]}.json`;
    a.click();
    URL.revokeObjectURL(url);
}

// Form submissions
document.getElementById('addSensorForm').addEventListener('submit', function(e) {
    e.preventDefault();
    alert('✅ Dodano sensor (symulacja)');
    closeModal('addSensorModal');
});

document.getElementById('addEffectorForm').addEventListener('submit', function(e) {
    e.preventDefault();
    alert('✅ Dodano effektor (symulacja)');
    closeModal('addEffectorModal');
});
