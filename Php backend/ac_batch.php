<?php
declare(strict_types=1);

file_put_contents(__DIR__ . "/debug.txt", "hit\n", FILE_APPEND);

header("Content-Type: application/json; charset=utf-8");

$raw = file_get_contents("php://input");
$len = strlen($raw);

if ($len < 4) {
    http_response_code(400);
    echo json_encode(["ok" => false, "error" => "Body too small"]);
    exit;
}

// count (uint32 LE)
$count = (int)unpack("Vcount", substr($raw, 0, 4))["count"];

// 8 + 4 + 4 + 33
$packetSize = 49;

if ($count > 5000) {
    http_response_code(400);
    echo json_encode(["ok" => false, "error" => "Invalid count", "count" => $count]);
    exit;
}

$expectedLen = 4 + ($count * $packetSize);

if ($len !== $expectedLen) {
    http_response_code(400);
    echo json_encode([
        "ok" => false,
        "error" => "Size mismatch",
        "expected" => $expectedLen,
        "got" => $len,
        "count" => $count
    ]);
    exit;
}

$FLAG_MAP = [
    "AC_FAKELAG_PATTERN"   => 1 << 0,
    "AC_DESYNC_POSSIBLE"   => 1 << 1,
    "AC_ROLL_ANGLE"        => 1 << 2,
    "AC_BACKTRACK"         => 1 << 3,
    "AC_TICKBASE_SHIFT"    => 1 << 4,
    "AC_FAKEDUCK"          => 1 << 5,
    "AC_RAGE_AIM"          => 1 << 6,
    "AC_MICROMOVE_PATTERN" => 1 << 7,
    "AC_ZERO_MOUSE"        => 1 << 8
];

$offset = 4;
$now = time();

for ($i = 0; $i < $count; $i++) {
    $chunk = substr($raw, $offset, $packetSize);

    $header = substr($chunk, 0, 16);
    $sessionRaw = substr($chunk, 16, 33);

    $p = unpack("Psteam_id/Vtick/Vflags", $header);

    $steam_id = $p["steam_id"];
    $tick = (int)$p["tick"];
    $flags = (int)$p["flags"];

    // remove null terminator/padding
    $session_id = rtrim($sessionRaw, "\0");

    // make filename safe
    $safe_session_id = preg_replace('/[^A-Za-z0-9_-]/', '_', $session_id);
    if ($safe_session_id === null || $safe_session_id === '') {
        $safe_session_id = 'unknown_session';
    }

    $row = [
        "ts" => $now,
        "session_id" => $session_id,
        "steam_id" => $steam_id,
        "tick" => $tick,
        "flags_raw" => $flags
    ];

    foreach ($FLAG_MAP as $name => $bit) {
        $row[$name] = (($flags & $bit) !== 0);
    }

    $logPath = __DIR__ . "/" . $safe_session_id . ".jsonl";

    file_put_contents(
        $logPath,
        json_encode($row, JSON_UNESCAPED_SLASHES) . PHP_EOL,
        FILE_APPEND
    );

    $offset += $packetSize;
}

echo json_encode([
    "ok" => true,
    "received" => $count
]);