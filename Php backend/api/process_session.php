<?php
declare(strict_types=1);

header("Content-Type: application/json; charset=utf-8");

$FLAGS = [
    "AC_FAKELAG_PATTERN",
    "AC_DESYNC_POSSIBLE",
    "AC_ROLL_ANGLE",
    "AC_BACKTRACK",
    "AC_TICKBASE_SHIFT",
    "AC_FAKEDUCK",
    "AC_RAGE_AIM",
    "AC_MICROMOVE_PATTERN",
    "AC_ZERO_MOUSE"
];

function load_jsonl_rows(string $path): array
{
    $rows = [];
    $fh = fopen($path, "r");
    if (!$fh) {
        return $rows;
    }

    while (($line = fgets($fh)) !== false) {
        $line = trim($line);
        if ($line === '') {
            continue;
        }

        $row = json_decode($line, true);
        if (is_array($row)) {
            $rows[] = $row;
        }
    }

    fclose($fh);
    return $rows;
}

function append_jsonl(string $path, array $row): bool
{
    $json = json_encode($row, JSON_UNESCAPED_SLASHES);
    if ($json === false) {
        return false;
    }

    return file_put_contents($path, $json . PHP_EOL, FILE_APPEND | LOCK_EX) !== false;
}

function ratio(int $count, int $packets): float
{
    if ($packets <= 0) {
        return 0.0;
    }

    return $count / $packets;
}

function load_state(string $path, string $sessionId, $steamId): array
{
    if (file_exists($path)) {
        $raw = file_get_contents($path);
        if ($raw !== false) {
            $decoded = json_decode($raw, true);
            if (is_array($decoded)) {
                return $decoded;
            }
        }
    }

    return [
        "session_id" => $sessionId,
        "steam_id" => $steamId,
        "cumulative_score" => 0,
        "suspicious_chunks" => 0,
        "processed_chunks" => 0,
        "banned" => false,
        "ban_record_written" => false,
        "ban_reasons" => [],
        "created_at" => time(),
        "updated_at" => time()
    ];
}

function save_state(string $path, array $state): bool
{
    $json = json_encode($state, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
    if ($json === false) {
        return false;
    }

    return file_put_contents($path, $json, LOCK_EX) !== false;
}

$session = $_GET["session"] ?? "";
$session = preg_replace('/[^A-Za-z0-9_-]/', '_', $session ?? '');

if ($session === null || $session === '') {
    http_response_code(400);
    echo json_encode(["ok" => false, "error" => "missing session"]);
    exit;
}

/*
 * process_session.php lives in htdocs/api/
 * session logs live in htdocs/
 */
$baseDir = dirname(__DIR__);

$sessionPath  = $baseDir . "/" . $session . ".jsonl";
$analysisPath = $baseDir . "/analysis_log.jsonl";
$banlistPath  = $baseDir . "/banlist.jsonl";

$stateDir  = $baseDir . "/session_state";
$statePath = $stateDir . "/" . $session . ".json";

if (!is_dir($stateDir)) {
    mkdir($stateDir, 0777, true);
}

if (!file_exists($sessionPath)) {
    http_response_code(404);
    echo json_encode([
        "ok" => false,
        "error" => "session log not found",
        "session" => $session,
        "path_checked" => $sessionPath
    ]);
    exit;
}

$rows = load_jsonl_rows($sessionPath);

if (empty($rows)) {
    http_response_code(400);
    echo json_encode([
        "ok" => false,
        "error" => "empty session log",
        "session" => $session
    ]);
    exit;
}

$steamId = $rows[0]["steam_id"] ?? null;
$total = count($rows);

$counts = array_fill_keys($FLAGS, 0);

foreach ($rows as $row) {
    foreach ($FLAGS as $flag) {
        if (!empty($row[$flag])) {
            $counts[$flag]++;
        }
    }
}

$ratios = [];
foreach ($counts as $flag => $count) {
    $ratios[$flag] = ratio($count, $total);
}

$chunkVerdict = "clean";
$chunkReasons = [];
$chunkScore = 0;

/*
 * Hard / high-confidence rules
 */
if ($ratios["AC_FAKEDUCK"] > 0.02) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_FAKEDUCK_ratio > 0.02";
}

if ($ratios["AC_BACKTRACK"] > 0.005) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_BACKTRACK_ratio > 0.005";
}

if ($ratios["AC_TICKBASE_SHIFT"] > 0.05) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_TICKBASE_SHIFT_ratio > 0.05";
}

if ($ratios["AC_DESYNC_POSSIBLE"] > 0.05) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_DESYNC_POSSIBLE_ratio > 0.05";
}

if ($ratios["AC_FAKELAG_PATTERN"] > 0.02) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_FAKELAG_PATTERN_ratio > 0.02";
}

if ($ratios["AC_ROLL_ANGLE"] > 0.02) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_ROLL_ANGLE_ratio > 0.02";
}

if ($ratios["AC_RAGE_AIM"] > 0.20) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_RAGE_AIM_ratio > 0.20";
}

if ($ratios["AC_ZERO_MOUSE"] > 0.003) {
    $chunkVerdict = "ban";
    $chunkReasons[] = "AC_ZERO_MOUSE_ratio > 0.003";
}

/*
 * Suspicion / support logic
 * Only applied if we did not already decide to ban.
 */
if ($chunkVerdict !== "ban") {
    /*
     * Leave out direct RAGE_AIM scoring:
     * it can be triggered by legit players with crazy high sensitivity.
     */

    if ($ratios["AC_ZERO_MOUSE"] > 0.0) {
        $chunkScore += 1;
        $chunkReasons[] = "AC_ZERO_MOUSE_ratio > 0.0";
    }

    if ($ratios["AC_MICROMOVE_PATTERN"] > 0.01) { // really good indicator
        $chunkScore += 1;
        $chunkReasons[] = "AC_MICROMOVE_PATTERN_ratio > 0.01";
    }

    if ($ratios["AC_RAGE_AIM"] > 0.01 && $ratios["AC_ZERO_MOUSE"] > 0.0) { // can save it as a combination
        $chunkScore += 2;
        $chunkReasons[] = "combo: RAGE_AIM + ZERO_MOUSE";
    }

    if ($ratios["AC_RAGE_AIM"] > 0.01 && $ratios["AC_MICROMOVE_PATTERN"] > 0.01) { // can save it as a combination
        $chunkScore += 1;
        $chunkReasons[] = "combo: RAGE_AIM + MICROMOVE_PATTERN";
    }

    if ($chunkScore >= 3) {
        $chunkVerdict = "suspicious";
    }
}

/*
 * Load and update persistent session state
 */
$state = load_state($statePath, $session, $steamId);

$state["updated_at"] = time();
$state["processed_chunks"] += 1;
$state["cumulative_score"] += $chunkScore;

if ($chunkVerdict === "suspicious") {
    $state["suspicious_chunks"] += 1;
}

if ($chunkVerdict === "ban") {
    $state["banned"] = true;
    $state["ban_reasons"] = array_values(array_unique(array_merge($state["ban_reasons"], $chunkReasons)));
}

/*
 * Session-level escalation logic
 */
if (!$state["banned"]) {
    if ($state["cumulative_score"] >= 6) {
        $state["banned"] = true;
        $state["ban_reasons"][] = "cumulative_score >= 6";
    }

    if ($state["suspicious_chunks"] >= 5) {
        $state["banned"] = true;
        $state["ban_reasons"][] = "suspicious_chunks >= 5";
    }

    $state["ban_reasons"] = array_values(array_unique($state["ban_reasons"]));
}

$finalVerdict = "clean";
if ($state["banned"]) {
    $finalVerdict = "ban";
} elseif ($state["cumulative_score"] > 0 || $state["suspicious_chunks"] > 0) {
    $finalVerdict = "suspicious";
}

$result = [
    "session_id" => $session,
    "steam_id" => $steamId,
    "packet_count" => $total,

    "chunk_verdict" => $chunkVerdict,
    "chunk_score" => $chunkScore,
    "chunk_reasons" => array_values(array_unique($chunkReasons)),

    "session_verdict" => $finalVerdict,
    "cumulative_score" => $state["cumulative_score"],
    "suspicious_chunks" => $state["suspicious_chunks"],
    "processed_chunks" => $state["processed_chunks"],
    "ban_reasons" => $state["ban_reasons"],

    "ratios" => $ratios,
    "counts" => $counts,
    "timestamp" => time(),
    "date_iso" => date("c")
];

$analysisSaved = append_jsonl($analysisPath, $result);

$banSaved = false;
if ($state["banned"] && !$state["ban_record_written"]) {
    $banSaved = append_jsonl($banlistPath, [
        "session_id" => $session,
        "steam_id" => $steamId,
        "verdict" => "ban",
        "cumulative_score" => $state["cumulative_score"],
        "suspicious_chunks" => $state["suspicious_chunks"],
        "processed_chunks" => $state["processed_chunks"],
        "ban_reasons" => $state["ban_reasons"],
        "timestamp" => time(),
        "date_iso" => date("c")
    ]);

    if ($banSaved) {
        $state["ban_record_written"] = true;
    }
}

$stateSaved = save_state($statePath, $state);
$deleted = @unlink($sessionPath);

echo json_encode([
    "ok" => true,
    "session" => $session,
    "steam_id" => $steamId,
    "packet_count" => $total,

    "chunk_verdict" => $chunkVerdict,
    "chunk_score" => $chunkScore,

    "session_verdict" => $finalVerdict,
    "cumulative_score" => $state["cumulative_score"],
    "suspicious_chunks" => $state["suspicious_chunks"],
    "processed_chunks" => $state["processed_chunks"],
    "ban_reasons" => $state["ban_reasons"],

    "reasons" => array_values(array_unique($chunkReasons)),
    "analysis_saved" => $analysisSaved,
    "ban_saved" => $banSaved,
    "state_saved" => $stateSaved,
    "log_deleted" => $deleted,
    "paths" => [
        "session" => $sessionPath,
        "analysis" => $analysisPath,
        "banlist" => $banlistPath,
        "state" => $statePath
    ]
], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);