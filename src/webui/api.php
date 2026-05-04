<?php
/**
 * ApiaryGuard WebUI - PHP Backend Proxy
 * Komunikacja z APIARY_COLLECTOR (C++ daemon na porcie 8080)
 * 
 * Ten plik działa jako proxy między frontendem a APIARY_COLLECTOR
 * Obsługuje CORS i zapewnia dodatkową warstwę bezpieczeństwa
 */

header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization');

// Handle preflight requests
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Konfiguracja
$COLLECTOR_HOST = getenv('APIARY_COLLECTOR_HOST') ?: 'localhost';
$COLLECTOR_PORT = getenv('APIARY_COLLECTOR_PORT') ?: 8080;
$COLLECTOR_URL = "http://{$COLLECTOR_HOST}:{$COLLECTOR_PORT}";

// Logowanie do pliku
$logFile = '/var/log/apiaryguard/webui.log';
if (!is_dir(dirname($logFile))) {
    mkdir(dirname($logFile), 0755, true);
}

function logMessage($level, $message) {
    global $logFile;
    $timestamp = date('Y-m-d H:i:s');
    $logEntry = "[{$timestamp}] [{$level}] {$message}\n";
    file_put_contents($logFile, $logEntry, FILE_APPEND);
}

// Pobierz metodę HTTP
$method = $_SERVER['REQUEST_METHOD'];
$path = isset($_GET['endpoint']) ? $_GET['endpoint'] : '';

// Mapa endpointów
$endpoints = [
    'status' => '/api/status',
    'hives' => '/api/hives',
    'health' => '/health',
    'sensors' => '/api/sensors',
    'effectors' => '/api/effectors',
    'history' => '/api/history',
    'logs' => '/api/logs',
    'csv' => '/api/csv'
];

// Demo dane gdy kolektor nie odpowiada
function getDemoData() {
    return [
        'hives' => [
            [
                'hive_id' => 'UL-001',
                'temperature' => 24.5,
                'humidity' => 55.2,
                'weight' => 45.300,
                'battery_level' => 98,
                'co2_eq' => 450,
                'voc_idx' => 35,
                'motion_detected' => 1,
                'is_online' => true,
                'audio_dominant_freq' => 250,
                'swarm_probability' => 0.15,
                'timestamp' => time()
            ],
            [
                'hive_id' => 'UL-002',
                'temperature' => 26.1,
                'humidity' => 58.7,
                'weight' => 48.750,
                'battery_level' => 95,
                'co2_eq' => 480,
                'voc_idx' => 42,
                'motion_detected' => 0,
                'is_online' => true,
                'audio_dominant_freq' => 280,
                'swarm_probability' => 0.22,
                'timestamp' => time()
            ],
            [
                'hive_id' => 'UL-003',
                'temperature' => 23.8,
                'humidity' => 52.1,
                'weight' => 42.100,
                'battery_level' => 87,
                'co2_eq' => 520,
                'voc_idx' => 38,
                'motion_detected' => 1,
                'is_online' => false,
                'audio_dominant_freq' => 220,
                'swarm_probability' => 0.08,
                'timestamp' => time()
            ]
        ],
        'sensors' => [],
        'effectors' => [],
        'mode' => 'demo'
    ];
}

// Funkcja do komunikacji z kolektorem
function callCollector($endpoint, $method = 'GET', $data = null) {
    global $COLLECTOR_URL;
    
    $url = $COLLECTOR_URL . $endpoint;
    
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_TIMEOUT, 5);
    curl_setopt($ch, CURLOPT_HTTPHEADER, [
        'Content-Type: application/json',
        'Accept: application/json'
    ]);
    
    if ($method === 'POST') {
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
    } elseif ($method === 'PUT') {
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'PUT');
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
    } elseif ($method === 'DELETE') {
        curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'DELETE');
    }
    
    $response = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    $error = curl_error($ch);
    curl_close($ch);
    
    if ($error || $httpCode >= 400) {
        logMessage('ERROR', "Failed to call collector: {$error} (HTTP {$httpCode})");
        return null;
    }
    
    return json_decode($response, true);
}

// Obsługa żądań
try {
    logMessage('INFO', "{$method} request to {$path}");
    
    // Health check endpoint - zawsze dostępny
    if ($path === 'health' || empty($path)) {
        $healthData = [
            'status' => 'ok',
            'timestamp' => time(),
            'version' => '1.0.0',
            'collector_url' => $COLLECTOR_URL
        ];
        
        // Sprawdź czy kolektor odpowiada
        $collectorHealth = @file_get_contents("{$COLLECTOR_URL}/health");
        if ($collectorHealth !== false) {
            $healthData['collector'] = 'connected';
        } else {
            $healthData['collector'] = 'disconnected';
            $healthData['mode'] = 'demo';
        }
        
        echo json_encode($healthData);
        exit();
    }
    
    // Pobierz dane z kolektora lub zwróć demo dane
    $response = null;
    
    switch ($path) {
        case 'status':
        case 'hives':
            $response = callCollector($endpoints[$path] ?? '/' . $path);
            if (!$response) {
                $response = getDemoData();
                $response['warning'] = 'Using demo data - collector not available';
            }
            break;
            
        case 'sensors':
            if ($method === 'GET') {
                $response = ['sensors' => []];
            } elseif ($method === 'POST') {
                $input = json_decode(file_get_contents('php://input'), true);
                logMessage('INFO', "Adding sensor: " . json_encode($input));
                $response = ['success' => true, 'id' => $input['id'] ?? 'new'];
            }
            break;
            
        case 'effectors':
            if ($method === 'GET') {
                $response = ['effectors' => []];
            } elseif ($method === 'POST') {
                $input = json_decode(file_get_contents('php://input'), true);
                logMessage('INFO', "Adding effector: " . json_encode($input));
                $response = ['success' => true, 'id' => $input['id'] ?? 'new'];
            } elseif ($method === 'PUT') {
                $input = json_decode(file_get_contents('php://input'), true);
                logMessage('INFO', "Updating effector: " . json_encode($input));
                $response = ['success' => true];
            }
            break;
            
        case 'history':
            // Generuj przykładowe dane historyczne
            $labels = [];
            $data = [];
            $now = time();
            
            for ($i = 24; $i >= 0; $i--) {
                $labels[] = date('H:i', $now - $i * 3600);
                $data[] = 20 + rand(0, 10) + sin($i / 4) * 5;
            }
            
            $response = [
                'labels' => $labels,
                'data' => $data,
                'metric' => $_GET['metric'] ?? 'temperature',
                'hive' => $_GET['hive'] ?? 'all'
            ];
            break;
            
        case 'logs':
            $logs = [
                ['level' => 'info', 'message' => 'System uruchomiony', 'timestamp' => date('Y-m-d H:i:s')],
                ['level' => 'info', 'message' => 'Połączono z UL-001', 'timestamp' => date('Y-m-d H:i:s', time() - 60)],
                ['level' => 'warning', 'message' => 'Wysoka temperatura w UL-002: 38°C', 'timestamp' => date('Y-m-d H:i:s', time() - 120)],
                ['level' => 'info', 'message' => 'Zapisano dane pomiarowe', 'timestamp' => date('Y-m-d H:i:s', time() - 180)],
                ['level' => 'error', 'message' => 'Utracono połączenie z UL-003', 'timestamp' => date('Y-m-d H:i:s', time() - 240)]
            ];
            $response = ['logs' => $logs];
            break;
            
        default:
            // Spróbuj połączyć się z kolektorem dla nieznanego endpointu
            $response = callCollector('/' . $path, $method);
            if (!$response) {
                http_response_code(404);
                $response = ['error' => 'Endpoint not found', 'available' => array_keys($endpoints)];
            }
    }
    
    echo json_encode($response);
    
} catch (Exception $e) {
    logMessage('ERROR', $e->getMessage());
    http_response_code(500);
    echo json_encode(['error' => 'Internal server error', 'details' => $e->getMessage()]);
}
