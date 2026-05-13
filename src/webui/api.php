<?php
/**
 * ApiaryGuard WebUI - PHP Backend Proxy
 * Komunikacja z APIARY_COLLECTOR (C++ daemon na porcie 8080)
 * 
 * Ten plik działa jako proxy między frontendem a APIARY_COLLECTOR
 * Obsługuje CORS i zapewnia dodatkową warstwę bezpieczeństwa
 * 
 * @package ApiaryGuard
 * @subpackage WebUI
 * @version 1.0.0
 * @author ApiaryGuard Pro Team
 * @license MIT
 * 
 * @debug DEBUG_MODE - ustaw true aby włączyć szczegółowe logowanie
 * @todo Dodac autoryzacje JWT
 * @todo Dodac rate limiting
 */

// ============================================================================
// KONFIGURACJA DEBUGOWANIA I LOGOWANIA
// ============================================================================
define('DEBUG_MODE', true); // Włącz/wyłącz tryb debugowania
define('LOG_ALL_REQUESTS', true); // Loguj wszystkie żądania HTTP
define('LOG_ERRORS_TO_FILE', true); // Loguj błędy do pliku
define('DISPLAY_ERRORS', false); // Nie wyświetlaj błędów użytkownikom (bezpieczeństwo)

// Raportowanie błędów - pełne dla debugowania
if (DEBUG_MODE) {
    error_reporting(E_ALL);
    ini_set('display_errors', DISPLAY_ERRORS ? '1' : '0');
    ini_set('log_errors', '1');
} else {
    error_reporting(0);
    ini_set('display_errors', '0');
}

// ============================================================================
// NAGŁÓWKI HTTP I CORS
// ============================================================================
header('Content-Type: application/json; charset=utf-8');

// Konfiguracja CORS - użyj zmiennej środowiskowej lub ogranicz do localhost w produkcji
$allowedOrigin = getenv('APIARY_CORS_ORIGIN') ?: 'http://localhost';
// W produkcji ustaw konkretną domenę zamiast '*'
// Przykład: $allowedOrigin = 'https://twojadomena.pl';
header("Access-Control-Allow-Origin: {$allowedOrigin}");
header('Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization, X-Requested-With, X-API-Key');
header('Access-Control-Max-Age: 86400'); // Cache preflight przez 24h
header('X-Content-Type-Options: nosniff'); // Bezpieczeństwo - zapobieg MIME sniffing
header('X-Frame-Options: DENY'); // Clickjacking protection
header('X-XSS-Protection: 1; mode=block'); // XSS filter
header('Referrer-Policy: strict-origin-when-cross-origin'); // Ochrona refererów
header('Permissions-Policy: geolocation=(), microphone=(), camera=()'); // Ograniczenie uprawnień

// Handle preflight requests
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Konfiguracja - używaj HTTPS w produkcji
$isProduction = getenv('APIARY_ENVIRONMENT') === 'production';
$COLLECTOR_HOST = getenv('APIARY_COLLECTOR_HOST') ?: 'localhost';
$COLLECTOR_PORT = (int)(getenv('APIARY_COLLECTOR_PORT') ?: 8080);
// Automatyczne wykrywanie protokołu - HTTPS w produkcji
$protocol = $isProduction ? 'https' : 'http';
$COLLECTOR_URL = "{$protocol}://{$COLLECTOR_HOST}:{$COLLECTOR_PORT}";

// ============================================================================
// KONFIGURACJA ŚCIEŻEK LOGÓW
// ============================================================================
$logFile = '/var/log/apiaryguard/webui.log';
$errorLogFile = '/var/log/apiaryguard/webui_errors.log';
$accessLogFile = '/var/log/apiaryguard/webui_access.log';

// Funkcja tworząca katalogi rekurencyjnie z obsługą błędów i walidacją ścieżki
function ensureDirectoryExists($path) {
    // Walidacja ścieżki - zapobiegaj directory traversal attacks
    if (strpos($path, '..') !== false || strpos($path, "\0") !== false) {
        error_log("[SECURITY ERROR] Invalid path detected: {$path}");
        return false;
    }
    
    if (!is_dir($path)) {
        try {
            if (!mkdir($path, 0755, true)) {
                error_log("[FILESYSTEM ERROR] Cannot create directory: {$path}");
                return false;
            }
            return true;
        } catch (Exception $e) {
            error_log("[EXCEPTION] Failed to create directory {$path}: " . $e->getMessage());
            return false;
        }
    }
    return true;
}

// Utwórz katalogi dla logów
ensureDirectoryExists(dirname($logFile));
ensureDirectoryExists(dirname($errorLogFile));
ensureDirectoryExists(dirname($accessLogFile));

// ============================================================================
// FUNKCJE LOGOWANIA Z OBSŁUGĄ WYJĄTKÓW I BŁĘDÓW
// ============================================================================

/**
 * Loguje wiadomość do pliku logu z poziomem i timestampem
 * 
 * @param string $level Poziom logu (DEBUG, INFO, WARNING, ERROR, CRITICAL)
 * @param string $message Treść wiadomości
 * @param string $source Źródło wiadomości (domyślnie 'WebUI')
 * @return bool Czy zapis się powiódł
 * 
 * @throws Exception Jeśli nie można zapisać do pliku logu
 */
function logMessage($level, $message, $source = 'WebUI') {
    global $logFile, $errorLogFile;
    
    // Sprawdź czy level jest poprawny
    $validLevels = ['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'];
    if (!in_array($level, $validLevels)) {
        $level = 'INFO'; // Domyślny poziom
    }
    
    $timestamp = date('Y-m-d H:i:s');
    $microtime = microtime(true);
    $pid = getmypid();
    
    // Format: [2024-01-15 10:30:45.123] [LEVEL] [PID:1234] [Source] Message
    $logEntry = "[{$timestamp}] [" . sprintf('%06.3f', $microtime - floor($microtime)) . "] [{$level}] [PID:{$pid}] [{$source}] {$message}\n";
    
    // Wybierz plik docelowy w zależności od poziomu
    $targetFile = ($level === 'ERROR' || $level === 'CRITICAL') ? $errorLogFile : $logFile;
    
    try {
        // Spróbuj zapisać z flagą FILE_APPEND | LOCK_EX dla thread-safety
        $result = file_put_contents($targetFile, $logEntry, FILE_APPEND | LOCK_EX);
        
        if ($result === false) {
            // Fallback: spróbuj bez locka
            $result = file_put_contents($targetFile, $logEntry, FILE_APPEND);
        }
        
        if ($result === false) {
            // Ostateczny fallback: loguj do stderr/system log
            error_log("[LOGGER FALLBACK] {$logEntry}");
            return false;
        }
        
        return true;
    } catch (Exception $e) {
        // Critical error - logger itself failed
        error_log("[LOGGER EXCEPTION] " . $e->getMessage() . " in " . $e->getFile() . ":" . $e->getLine());
        return false;
    }
}

/**
 * Loguje żądanie HTTP do pliku access log
 * 
 * @param array $_server Tablica $_SERVER z danymi żądania
 * @return void
 */
function logHttpRequest($_server) {
    global $accessLogFile;
    
    if (!defined('LOG_ALL_REQUESTS') || !LOG_ALL_REQUESTS) {
        return;
    }
    
    $method = $_server['REQUEST_METHOD'] ?? 'UNKNOWN';
    $uri = $_server['REQUEST_URI'] ?? '/';
    $ip = $_server['REMOTE_ADDR'] ?? 'unknown';
    $userAgent = $_server['HTTP_USER_AGENT'] ?? 'unknown';
    $referer = $_server['HTTP_REFERER'] ?? '-';
    
    $timestamp = date('Y-m-d H:i:s');
    $accessEntry = "[{$timestamp}] {$ip} - - \"{$method} {$uri} HTTP/1.1\" 200 - \"{$referer}\" \"{$userAgent}\"\n";
    
    try {
        file_put_contents($accessLogFile, $accessEntry, FILE_APPEND | LOCK_EX);
    } catch (Exception $e) {
        error_log("[ACCESS LOG ERROR] " . $e->getMessage());
    }
}

/**
 * Obsługuje wyjątki i błędy fatalne
 * 
 * @param Throwable $exception Wyjątek lub błąd
 * @return void
 */
function handleException($exception) {
    logMessage('CRITICAL', 
        "Unhandled exception: " . $exception->getMessage() . 
        " in " . $exception->getFile() . ":" . $exception->getLine() .
        " Stack trace: " . $exception->getTraceAsString(),
        'ExceptionHandler'
    );
    
    if (!headers_sent()) {
        http_response_code(500);
        echo json_encode([
            'error' => 'Internal server error',
            'type' => get_class($exception),
            'file' => DEBUG_MODE ? $exception->getFile() : null,
            'line' => DEBUG_MODE ? $exception->getLine() : null,
            'message' => DEBUG_MODE ? $exception->getMessage() : 'An unexpected error occurred'
        ]);
    }
}

// Zarejestruj handler wyjątków
set_exception_handler('handleException');

/**
 * Obsługuje błędy PHP
 * 
 * @param int $errno Poziom błędu
 * @param string $errstr Wiadomość błędu
 * @param string $errfile Plik gdzie wystąpił błąd
 * @param int $errline Linia błędu
 * @return bool
 */
function handleError($errno, $errstr, $errfile, $errline) {
    $levelMap = [
        E_ERROR => 'ERROR',
        E_WARNING => 'WARNING',
        E_PARSE => 'CRITICAL',
        E_NOTICE => 'DEBUG',
        E_CORE_ERROR => 'CRITICAL',
        E_CORE_WARNING => 'ERROR',
        E_COMPILE_ERROR => 'CRITICAL',
        E_COMPILE_WARNING => 'ERROR',
        E_USER_ERROR => 'ERROR',
        E_USER_WARNING => 'WARNING',
        E_USER_NOTICE => 'DEBUG',
        E_STRICT => 'DEBUG',
        E_RECOVERABLE_ERROR => 'ERROR',
        E_DEPRECATED => 'DEBUG',
        E_USER_DEPRECATED => 'DEBUG'
    ];
    
    $level = $levelMap[$errno] ?? 'INFO';
    $message = "[{$errno}] {$errstr} in {$errfile}:{$errline}";
    
    logMessage($level, $message, 'ErrorHandler');
    
    // Nie przerywaj działania dla warningów i notice
    return ($errno !== E_ERROR && $errno !== E_USER_ERROR);
}

// Zarejestruj handler błędów
set_error_handler('handleError');

// Zarejestruj shutdown function dla błędów fatalnych
register_shutdown_function(function() {
    $error = error_get_last();
    if ($error !== null && in_array($error['type'], [E_ERROR, E_PARSE, E_CORE_ERROR, E_COMPILE_ERROR])) {
        logMessage('CRITICAL', 
            "Fatal error: {$error['message']} in {$error['file']}:{$error['line']}",
            'ShutdownHandler'
        );
    }
});

// Pobierz metodę HTTP z walidacją
$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
$validMethods = ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'];
if (!in_array(strtoupper($method), $validMethods)) {
    logMessage('WARNING', "Invalid HTTP method: {$method}", 'RequestValidator');
    http_response_code(405); // Method Not Allowed
    echo json_encode(['error' => 'Method not allowed', 'allowed_methods' => $validMethods]);
    exit();
}

// Pobierz endpoint z walidacją - BEZ htmlspecialchars (to API JSON, nie HTML)
// htmlspecialchars jest tylko do outputu HTML, nie do ścieżek API
$path = isset($_GET['endpoint']) ? trim($_GET['endpoint']) : '';

// Walidacja ścieżki - dozwolone znaki: a-z, 0-9, /, -, _
if ($path && !preg_match('/^[a-zA-Z0-9\/\-_]+$/', $path)) {
    logMessage('WARNING', "Invalid endpoint path: {$path}", 'Security');
    http_response_code(400);
    echo json_encode(['error' => 'Invalid endpoint path', 'allowed_chars' => 'a-z, 0-9, /, -, _']);
    exit();
}

// Zaloguj żądanie HTTP
logHttpRequest($_SERVER);
logMessage('DEBUG', "Incoming request: {$method} {$path}", 'RequestHandler');

// Mapa endpointów - centralna rejestracja wszystkich dostępnych endpointów
$endpoints = [
    'status' => '/api/status',
    'hives' => '/api/hives',
    'health' => '/health',
    'sensors' => '/api/sensors',
    'effectors' => '/api/effectors',
    'history' => '/api/history',
    'logs' => '/api/logs',
    'csv' => '/api/csv',
    'config' => '/api/config'
];

/**
 * Waliduje nazwę endpointu
 * 
 * @param string $path Nazwa endpointu do walidacji
 * @return bool Czy endpoint jest poprawny
 */
function isValidEndpoint($path) {
    global $endpoints;
    return array_key_exists($path, $endpoints) || empty($path) || $path === 'health';
}

// Sprawdź czy endpoint jest poprawny
if (!isValidEndpoint($path)) {
    logMessage('WARNING', "Invalid endpoint requested: {$path}", 'Security');
    http_response_code(400);
    echo json_encode([
        'error' => 'Invalid endpoint',
        'available_endpoints' => array_keys($endpoints),
        'requested' => $path
    ]);
    exit();
}

// Ścieżka do plików konfiguracyjnych z walidacją
$configDir = getenv('HOME') ? rtrim(getenv('HOME'), '/') . '/.hive_monitor' : '/root/.hive_monitor';

/**
 * Bezpiecznie tworzy katalog konfiguracyjny
 * 
 * @param string $path Ścieżka do katalogu
 * @return bool Czy operacja się powiodła
 */
function ensureConfigDir($path) {
    try {
        // Walidacja ścieżki - zapobiegaj directory traversal
        if (strpos($path, '..') !== false || strpos($path, "\0") !== false) {
            logMessage('ERROR', "Invalid config path detected: {$path}", 'Security');
            return false;
        }
        
        if (!is_dir($path)) {
            if (!mkdir($path, 0755, true)) {
                logMessage('ERROR', "Cannot create config directory: {$path}", 'Filesystem');
                return false;
            }
            logMessage('INFO', "Created config directory: {$path}", 'Filesystem');
        }
        return true;
    } catch (Exception $e) {
        logMessage('ERROR', "Exception creating config dir: " . $e->getMessage(), 'Filesystem');
        return false;
    }
}

ensureConfigDir($configDir);

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

// Funkcja do wczytywania konfiguracji z plików .conf
function loadConfig() {
    global $configDir;
    $config = [];
    
    $configFiles = [
        'api_endpoints.conf' => 'api',
        'database_settings.conf' => 'database',
        'update_intervals.conf' => 'intervals',
        'notifications.conf' => 'notifications',
        'data_retention.conf' => 'retention',
        'network_settings.conf' => 'network',
        'user_preferences.conf' => 'preferences',
        'advanced_options.conf' => 'advanced'
    ];
    
    foreach ($configFiles as $file => $section) {
        $filePath = $configDir . '/' . $file;
        if (file_exists($filePath)) {
            $lines = file($filePath, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
            foreach ($lines as $line) {
                $line = trim($line);
                if (empty($line) || $line[0] === '#') continue;
                
                if (strpos($line, '=') !== false) {
                    list($key, $value) = explode('=', $line, 2);
                    $key = trim($key);
                    $value = trim($value, " \"'");
                    
                    // Konwertuj wartości na odpowiednie typy
                    if ($value === 'true' || $value === 'false') {
                        $value = ($value === 'true');
                    } elseif (is_numeric($value)) {
                        // Sprawdź czy liczba jest całkowita - użyj porównania stringów dla precyzji
                        $floatValue = floatval($value);
                        // Porównaj reprezentację stringową aby uniknąć błędów konwersji float->int
                        if (strpos($value, '.') === false && $floatValue == intval($floatValue)) {
                            $value = intval($floatValue);
                        } else {
                            $value = $floatValue;
                        }
                    }
                    
                    $config[$section][$key] = $value;
                }
            }
        }
    }
    
    return $config;
}

// Funkcja do zapisywania konfiguracji
function saveConfig($configData) {
    global $configDir;
    
    if (!is_dir($configDir)) {
        mkdir($configDir, 0755, true);
    }
    
    $sectionMap = [
        'api' => 'api_endpoints.conf',
        'database' => 'database_settings.conf',
        'intervals' => 'update_intervals.conf',
        'notifications' => 'notifications.conf',
        'retention' => 'data_retention.conf',
        'network' => 'network_settings.conf',
        'preferences' => 'user_preferences.conf',
        'advanced' => 'advanced_options.conf'
    ];
    
    $results = [];
    foreach ($configData as $section => $values) {
        if (!isset($sectionMap[$section])) continue;
        
        $filePath = $configDir . '/' . $sectionMap[$section];
        $content = "# Hive Monitor Configuration - {$section}\n";
        $content .= "# Generated: " . date('Y-m-d H:i:s') . "\n\n";
        
        foreach ($values as $key => $value) {
            if (is_bool($value)) {
                $value = $value ? 'true' : 'false';
            }
            $content .= "{$key}={$value}\n";
        }
        
        $success = file_put_contents($filePath, $content);
        $results[$section] = ($success !== false);
    }
    
    return $results;
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
            
        case 'config':
            // Obsługa konfiguracji - odczyt i zapis
            if ($method === 'GET') {
                // Pobierz całą konfigurację
                $config = loadConfig();
                $response = ['config' => $config, 'source' => 'files'];
            } elseif ($method === 'POST') {
                // Zapisz konfigurację
                $input = json_decode(file_get_contents('php://input'), true);
                if ($input && isset($input['config'])) {
                    $results = saveConfig($input['config']);
                    $response = ['success' => true, 'saved' => $results, 'message' => 'Konfiguracja zapisana'];
                } else {
                    http_response_code(400);
                    $response = ['error' => 'Invalid config data'];
                }
            } elseif ($method === 'PUT') {
                // Aktualizuj wybraną sekcję konfiguracji
                $input = json_decode(file_get_contents('php://input'), true);
                if ($input) {
                    $results = saveConfig($input);
                    $response = ['success' => true, 'saved' => $results, 'message' => 'Konfiguracja zaktualizowana'];
                } else {
                    http_response_code(400);
                    $response = ['error' => 'Invalid config data'];
                }
            }
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
