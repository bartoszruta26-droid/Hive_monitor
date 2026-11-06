<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: no-store, private');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: no-referrer');

function legacyApiRespond(int $status, array $payload): never {
    http_response_code($status);
    echo json_encode($payload, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') !== 'GET') {
    header('Allow: GET');
    legacyApiRespond(405, ['error' => 'method_not_allowed']);
}

$keyFile = getenv('APIARY_API_KEY_FILE') ?: '/etc/apiaryguard/apiary_api_key';
$configuredKey = getenv('APIARY_API_KEY') ?: '';
if ($configuredKey === '' && is_readable($keyFile)) {
    $configuredKey = trim((string)file_get_contents($keyFile));
}
if ($configuredKey === '') {
    legacyApiRespond(503, ['error' => 'api_access_disabled']);
}
$providedKey = $_SERVER['HTTP_X_API_KEY'] ?? '';
if ($providedKey === '' || !hash_equals($configuredKey, $providedKey)) {
    legacyApiRespond(401, ['error' => 'unauthorized']);
}

$action = $_GET['action'] ?? 'latest';
$actionTypes = [
    'history_minute' => 1,
    'history_hour' => 2,
    'history_day' => 3,
    'history_week' => 4,
    'history_month' => 5,
    'history_quarter' => 6,
    'history_year' => 7
];
$allowedActions = array_merge(['latest', 'raw', 'stats'], array_keys($actionTypes));
if (!is_string($action) || !in_array($action, $allowedActions, true)) {
    legacyApiRespond(400, ['error' => 'invalid_action']);
}

$hiveId = $_GET['hive_id'] ?? '';
if (!is_string($hiveId) || ($hiveId !== '' &&
    (!preg_match('/^[A-Za-z0-9._-]{1,64}$/D', $hiveId)))) {
    legacyApiRespond(400, ['error' => 'invalid_hive_id']);
}

$limitRaw = $_GET['limit'] ?? '100';
if (!is_string($limitRaw) && !is_int($limitRaw)) {
    legacyApiRespond(400, ['error' => 'invalid_limit', 'min' => 1, 'max' => 1000]);
}
$limit = filter_var($limitRaw, FILTER_VALIDATE_INT);
if ($limit === false || $limit < 1 || $limit > 1000) {
    legacyApiRespond(400, ['error' => 'invalid_limit', 'min' => 1, 'max' => 1000]);
}

$dbPath = getenv('APIARY_DB_PATH') ?: '/var/lib/apiaryguard/apiary.db';
if (!is_file($dbPath) || !is_readable($dbPath)) {
    legacyApiRespond(503, ['error' => 'database_unavailable']);
}

try {
    $db = new SQLite3($dbPath, SQLITE3_OPEN_READONLY);
    $db->enableExceptions(true);
    $db->busyTimeout(3000);

    if ($action === 'stats') {
        $stats = [];
        $stats['raw_count'] = $db->querySingle('SELECT COUNT(*) FROM raw_data');
        for ($type = 1; $type <= 7; ++$type) {
            $names = [1 => 'minute', 2 => 'hour', 3 => 'day', 4 => 'week',
                      5 => 'month', 6 => 'quarter', 7 => 'year'];
            $stats[$names[$type] . '_count'] = $db->querySingle(
                'SELECT COUNT(*) FROM aggregated_data WHERE agg_type = ' . $type
            );
        }
        $stats['oldest_raw'] = $db->querySingle(
            "SELECT datetime(timestamp, 'unixepoch') FROM raw_data ORDER BY timestamp ASC LIMIT 1"
        );
        $stats['newest_raw'] = $db->querySingle(
            "SELECT datetime(timestamp, 'unixepoch') FROM raw_data ORDER BY timestamp DESC LIMIT 1"
        );
        $db->close();
        legacyApiRespond(200, $stats);
    }

    $isRaw = $action === 'raw';
    $table = $isRaw ? 'raw_data' : 'aggregated_data';
    $where = [];
    if ($isRaw) {
        $where[] = "timestamp > (strftime('%s', 'now') - 604800)";
    } else {
        $type = $action === 'latest' ? 1 : $actionTypes[$action];
        $where[] = 'agg_type = ' . (int)$type;
    }
    if ($hiveId !== '') {
        $where[] = 'hive_id = :hive_id';
    }
    $timeColumn = $isRaw ? 'timestamp' : 'timestamp_start';
    $sql = 'SELECT * FROM ' . $table . ' WHERE ' . implode(' AND ', $where) .
           ' ORDER BY ' . $timeColumn . ' DESC LIMIT :limit';
    $stmt = $db->prepare($sql);
    if ($hiveId !== '') {
        $stmt->bindValue(':hive_id', $hiveId, SQLITE3_TEXT);
    }
    $stmt->bindValue(':limit', $action === 'latest' ? 1 : $limit, SQLITE3_INTEGER);
    $result = $stmt->execute();
    $rows = [];
    while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
        $rows[] = $row;
    }
    $result->finalize();
    $stmt->close();
    $db->close();

    if ($action === 'latest') {
        legacyApiRespond(200, $rows[0] ?? ['status' => 'no_data']);
    }
    legacyApiRespond(200, $rows);
} catch (Throwable $exception) {
    error_log('[Apiary legacy API] ' . $exception->getMessage());
    legacyApiRespond(500, ['error' => 'internal_server_error']);
}
