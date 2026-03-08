<?php
declare(strict_types=1);

header("Content-Type: application/json; charset=utf-8");
header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
header("Pragma: no-cache");
header("Expires: 0");

$session = $_GET["session"] ?? "";
$session = preg_replace('/[^A-Za-z0-9_-]/', '_', $session ?? '');

if ($session === '') {
    http_response_code(400);
    echo json_encode([
        "ok" => false,
        "banned" => false,
        "error" => "missing_session"
    ], JSON_UNESCAPED_SLASHES);
    exit;
}

$baseDir = dirname(__DIR__);
$statePath = $baseDir . "/session_state/" . $session . ".json";

/*
 * No state file = definitely not banned
 */
if (!is_file($statePath)) {
    echo json_encode([
        "ok" => true,
        "banned" => false,
        "reason" => "no_state",
        "session" => $session
    ], JSON_UNESCAPED_SLASHES);
    exit;
}

$raw = @file_get_contents($statePath);

/*
 * Can't read state = treat as not banned
 */
if ($raw === false || trim($raw) === '') {
    echo json_encode([
        "ok" => true,
        "banned" => false,
        "reason" => "state_unreadable",
        "session" => $session
    ], JSON_UNESCAPED_SLASHES);
    exit;
}

$state = json_decode($raw, true);

/*
 * Corrupted state = treat as not banned
 */
if (!is_array($state)) {
    echo json_encode([
        "ok" => true,
        "banned" => false,
        "reason" => "state_corrupted",
        "session" => $session
    ], JSON_UNESCAPED_SLASHES);
    exit;
}

/*
 * ONLY exact boolean true means banned.
 * Strings like "true", "false", 1, "1", etc. do NOT count.
 */
$banned = array_key_exists("banned", $state) && $state["banned"] === true;

echo json_encode([
    "ok" => true,
    "banned" => $banned,
    "session" => $session,
    "score" => isset($state["cumulative_score"]) ? (int)$state["cumulative_score"] : 0,
    "suspicious_chunks" => isset($state["suspicious_chunks"]) ? (int)$state["suspicious_chunks"] : 0,
    "reasons" => isset($state["ban_reasons"]) && is_array($state["ban_reasons"]) ? $state["ban_reasons"] : []
], JSON_UNESCAPED_SLASHES);