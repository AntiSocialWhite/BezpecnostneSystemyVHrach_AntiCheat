<?php
declare(strict_types=1);

header("Content-Type: text/plain; charset=utf-8");

$debugFile = __DIR__ . "/check_debug.txt";

function log_dbg($msg) {
    global $debugFile;
    file_put_contents($debugFile, date("H:i:s") . " | " . $msg . "\n", FILE_APPEND);
}

log_dbg("REQUEST START");
log_dbg("REQUEST_URI: " . ($_SERVER["REQUEST_URI"] ?? ""));
log_dbg("QUERY_STRING: " . ($_SERVER["QUERY_STRING"] ?? ""));
log_dbg("PHP_SELF: " . ($_SERVER["PHP_SELF"] ?? ""));
log_dbg("RAW session: [" . ($_GET["session"] ?? "") . "]");

$session = $_GET["session"] ?? "";

if ($session === "") {
    log_dbg("BAD: empty session");
    echo "BAD";
    exit;
}

$session = preg_replace('/[^A-Za-z0-9_-]/', '_', $session);
log_dbg("SANITIZED session: [" . $session . "]");

$path = __DIR__ . "/heartbeat/" . $session . ".txt";
log_dbg("checking file: " . $path);

if (!file_exists($path)) {
    log_dbg("WAIT: file not found");
    echo "WAIT";
    exit;
}

$raw = file_get_contents($path);
if ($raw === false) {
    log_dbg("BAD: file read failed");
    echo "BAD";
    exit;
}

$lastSeen = (int)trim($raw);
$age = time() - $lastSeen;

log_dbg("age: " . $age);

if ($age <= 15) {
    log_dbg("RESULT: OK");
    echo "OK";
} else {
    log_dbg("RESULT: BAD");
    echo "BAD";
}