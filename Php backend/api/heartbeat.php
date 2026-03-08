<?php
declare(strict_types=1);

$debug = __DIR__ . "/heartbeat_debug.txt";
file_put_contents($debug, "hit\n", FILE_APPEND);

header("Content-Type: text/plain; charset=utf-8");

$session = $_GET["session"] ?? "";
file_put_contents($debug, "session_raw=" . $session . "\n", FILE_APPEND);

if ($session === "") {
    file_put_contents($debug, "BAD empty session\n", FILE_APPEND);
    echo "BAD";
    exit;
}

$session = preg_replace('/[^A-Za-z0-9_-]/', '_', $session);
file_put_contents($debug, "session_sanitized=" . $session . "\n", FILE_APPEND);

if ($session === null || $session === "") {
    file_put_contents($debug, "BAD sanitized empty\n", FILE_APPEND);
    echo "BAD";
    exit;
}

$dir = __DIR__ . "/heartbeat";
if (!is_dir($dir)) {
    $ok = mkdir($dir, 0777, true);
    file_put_contents($debug, "mkdir=" . ($ok ? "ok" : "fail") . "\n", FILE_APPEND);
}

$path = $dir . "/" . $session . ".txt";
file_put_contents($debug, "path=" . $path . "\n", FILE_APPEND);

$result = file_put_contents($path, (string)time());
file_put_contents($debug, "write_result=" . var_export($result, true) . "\n", FILE_APPEND);

if ($result === false) {
    echo "BAD";
    exit;
}

echo "OK";