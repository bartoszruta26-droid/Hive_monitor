<?php
/**
 * HTTP-level regression tests for WebUI API authentication and configuration.
 * Run: php tests/php/api_security_config_test.php
 */

function assertSameValue($expected, $actual, string $message): void {
    if ($expected !== $actual) {
        throw new RuntimeException($message . ': expected ' . var_export($expected, true) .
            ', got ' . var_export($actual, true));
    }
}

function reservePort(): int {
    $socket = stream_socket_server('tcp://127.0.0.1:0', $errno, $error);
    if ($socket === false) {
        throw new RuntimeException("Cannot reserve test port: {$error}");
    }
    $address = stream_socket_get_name($socket, false);
    fclose($socket);
    return (int)substr(strrchr($address, ':'), 1);
}

function startApiServer(string $home, string $apiKey, int $collectorPort = 1): array {
    $port = reservePort();
    putenv('HOME=' . $home);
    putenv('APIARY_API_KEY=' . $apiKey);
    putenv('APIARY_CORS_ORIGIN=http://127.0.0.1');
    putenv('APIARY_ENVIRONMENT=development');
    putenv('APIARY_COLLECTOR_HOST=127.0.0.1');
    putenv('APIARY_COLLECTOR_PORT=' . $collectorPort);
    $root = dirname(__DIR__, 2);
    $command = [PHP_BINARY, '-S', "127.0.0.1:{$port}", '-t', $root . '/src/webui'];
    $process = proc_open($command, [
        0 => ['pipe', 'r'],
        1 => ['file', '/dev/null', 'a'],
        2 => ['file', '/dev/null', 'a']
    ], $pipes, $root);
    if (!is_resource($process)) {
        throw new RuntimeException('Cannot start PHP development server');
    }
    fclose($pipes[0]);

    $base = "http://127.0.0.1:{$port}/api.php?endpoint=config";
    $ready = false;
    for ($i = 0; $i < 50; $i++) {
        $context = stream_context_create(['http' => ['timeout' => 0.2]]);
        @file_get_contents($base, false, $context);
        if (isset($http_response_header[0])) {
            $ready = true;
            break;
        }
        usleep(100000);
    }
    if (!$ready) {
        proc_terminate($process);
        proc_close($process);
        throw new RuntimeException('PHP API server did not become ready');
    }
    return [$process, $base];
}

function requestApi(string $url, string $method, ?array $body = null, ?string $key = null): array {
    $headers = ['Accept: application/json'];
    if ($key !== null) {
        $headers[] = 'X-API-Key: ' . $key;
    }
    $options = [
        'method' => $method,
        'ignore_errors' => true,
        'timeout' => 6,
        'header' => implode("\r\n", $headers)
    ];
    if ($body !== null) {
        $options['header'] .= "\r\nContent-Type: application/json";
        $options['content'] = json_encode($body, JSON_THROW_ON_ERROR);
    }
    $context = stream_context_create(['http' => $options]);
    $response = file_get_contents($url, false, $context);
    $statusLine = $http_response_header[0] ?? '';
    preg_match('/\s([0-9]{3})\s/', $statusLine, $matches);
    return [(int)($matches[1] ?? 0), $response === false ? '' : $response];
}

$home = sys_get_temp_dir() . '/hive-monitor-api-test-' . bin2hex(random_bytes(6));
if (!mkdir($home, 0700, true)) {
    throw new RuntimeException('Cannot create isolated test HOME');
}

$server = null;
$mock = null;
$mockRouter = null;
try {
    // Missing server key must fail closed.
    [$server, $url] = startApiServer($home, '');
    [$status] = requestApi($url, 'GET');
    assertSameValue(503, $status, 'GET config without configured API key');
    proc_terminate($server);
    proc_close($server);
    $server = null;

    // Wrong and missing client keys are rejected; the correct key works.
    [$server, $url] = startApiServer($home, 'test-secret-key');
    [$status] = requestApi($url, 'GET');
    assertSameValue(401, $status, 'GET config without client API key');
    [$status] = requestApi($url, 'GET', null, 'wrong-key');
    assertSameValue(401, $status, 'GET config with incorrect API key');
    [$status, $body] = requestApi($url, 'GET', null, 'test-secret-key');
    assertSameValue(200, $status, 'GET config with correct API key');
    assertSameValue(true, isset(json_decode($body, true)['config']), 'Authenticated config response');

    [$status] = requestApi($url, 'POST', ['config' => ['intervals' => ['poll_seconds' => 30]]], 'test-secret-key');
    assertSameValue(200, $status, 'Valid configuration write');
    $configFile = $home . '/.hive_monitor/update_intervals.conf';
    assertSameValue(true, is_file($configFile), 'Configuration file created');
    $before = file_get_contents($configFile);

    [$status] = requestApi($url, 'POST', ['config' => ['intervals' => ['poll_seconds' => "30\nadmin=true"]]], 'test-secret-key');
    assertSameValue(400, $status, 'Newline injection rejected');
    assertSameValue($before, file_get_contents($configFile), 'Rejected input leaves prior configuration unchanged');

    [$status] = requestApi($url, 'PUT', ['unknown_section' => ['enabled' => true]], 'test-secret-key');
    assertSameValue(400, $status, 'Unknown configuration section rejected');

    [$status] = requestApi($url, 'PUT', ['intervals' => ['bad key' => 1]], 'test-secret-key');
    assertSameValue(400, $status, 'Invalid configuration key rejected');

    proc_terminate($server);
    proc_close($server);
    $server = null;

    // A stalled collector must not hold the WebUI health request indefinitely.
    $mockPort = reservePort();
    $mockRouter = $home . '/slow-collector.php';
    file_put_contents($mockRouter, "<?php usleep(5000000); header('Content-Type: application/json'); echo '{}';");
    $mock = proc_open([PHP_BINARY, '-S', "127.0.0.1:{$mockPort}", $mockRouter], [
        0 => ['pipe', 'r'],
        1 => ['file', '/dev/null', 'a'],
        2 => ['file', '/dev/null', 'a']
    ], $mockPipes, $home);
    if (!is_resource($mock)) {
        throw new RuntimeException('Cannot start slow collector fixture');
    }
    fclose($mockPipes[0]);
    usleep(200000);
    [$server, $url] = startApiServer($home, 'test-secret-key', $mockPort);
    $healthUrl = str_replace('endpoint=config', 'endpoint=health', $url);
    $startedAt = microtime(true);
    [$status, $healthBody] = requestApi($healthUrl, 'GET');
    $elapsed = microtime(true) - $startedAt;
    assertSameValue(200, $status, 'WebUI health endpoint remains responsive');
    $health = json_decode($healthBody, true);
    assertSameValue('disconnected', $health['collector'] ?? null, 'Stalled collector is marked disconnected');
    assertSameValue(true, $elapsed < 4.5, 'Collector health check respects bounded timeout');
    proc_terminate($mock);
    proc_close($mock);
    $mock = null;

    echo "PHP API authentication, configuration and timeout regression tests passed\n";
} finally {
    if (is_resource($server)) {
        proc_terminate($server);
        proc_close($server);
    }
    if (is_resource($mock)) {
        proc_terminate($mock);
        proc_close($mock);
    }
    if ($mockRouter !== null && is_file($mockRouter)) {
        @unlink($mockRouter);
    }
    putenv('APIARY_API_KEY');
    putenv('APIARY_CORS_ORIGIN');
    putenv('APIARY_ENVIRONMENT');
    putenv('APIARY_COLLECTOR_HOST');
    putenv('APIARY_COLLECTOR_PORT');
    putenv('HOME');
    $files = glob($home . '/.hive_monitor/*') ?: [];
    foreach ($files as $file) {
        @unlink($file);
    }
    @rmdir($home . '/.hive_monitor');
    @rmdir($home);
}
