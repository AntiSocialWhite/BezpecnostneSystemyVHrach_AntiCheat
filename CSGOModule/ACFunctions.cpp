#include "DllModule.h"
#include <iostream>

// These functions are based on info I have gathered over the last few years creating cheats for CS:GO
// and understanding how different exploits / cheats work.
// This should ban any rage hackers in no time, unlike VAC.

static inline float AngleDiff(float a, float b) {
	float d = a - b;
	while (d > 180.f) d -= 360.f;
	while (d < -180.f) d += 360.f;
	return d;
}

bool AC_Funcs::using_fakelag() {
	if (!interfaces::clientstate)
		return false;

	const int choke = interfaces::clientstate->iChokedCommands;

	if (choke > info.current_choke_peak)
		info.current_choke_peak = choke;

	bool suspicious_cycle = false;

	// completed choke cycle
	if (info.last_choke_cycle > 0 && choke == 0)
	{
		const int completed_peak = info.current_choke_peak;

		if (completed_peak >= 4)
		{
			suspicious_cycle = true;

			// stronger signal if peaks are roughly similar
			if (info.last_choke_peak > 0 && abs(completed_peak - info.last_choke_peak) <= 2)
				info.fakelag_score += 2;
			else
				info.fakelag_score += 1;

			info.last_choke_peak = completed_peak;
		}

		info.current_choke_peak = 0;
	}

	// decay score if no suspicious cycle happened recently
	if (!suspicious_cycle)
	{
		info.fakelag_cooldown++;

		if (info.fakelag_cooldown >= 16)
		{
			if (info.fakelag_score > 0)
				info.fakelag_score--;

			info.fakelag_cooldown = 0;
		}
	}
	else
	{
		info.fakelag_cooldown = 0;
	}

	info.last_choke_cycle = choke;

	return info.fakelag_score >= 4;
}

bool AC_Funcs::using_micromovement(c_usercmd* cmd) { // this works together with possible_desync
	if (!cmd || !info.prev_cmd)
		return false;

	// Standing-only gate (intent-based)
	const bool ducking = (cmd->buttons & (1 << 2)); // in duck = (1 << 2)
	const bool jumping = (cmd->buttons & (1 << 1)); // in jump = (1 << 1)

	if (jumping)
	{
		info.ticks_sidemoving = 0; return false;
	}

	// Player isn't trying to move forward; micromove only nudges sidemove
	if (fabs(cmd->forwardmove) > 5.0f) // small tolerance for noise
	{
		info.ticks_sidemoving = 0; return false;
	}

	const float target = ducking ? 3.4f : 1.1f;
	const float tolMag = 0.20f;   // accept 0.9..1.3-ish
	const float epsFlip = 0.15f;  // how close cmd & prevcmd must be
	const int max_treshold = 16; // how many consecutive ticks we need to be sure - createmove runs every tick.

	const float cur = cmd->sidemove;
	const float prev = info.prev_cmd->sidemove;

	const bool mag_ok =
		fabs(fabs(cur) - target) < tolMag &&
		fabs(fabs(prev) - target) < tolMag;

	const bool flips_clean = fabs(cur + prev) < epsFlip;

	if (mag_ok && flips_clean) {
		info.ticks_sidemoving++;
	}
	else {
		info.ticks_sidemoving = 0;
	}

	return info.ticks_sidemoving >= max_treshold;
}

/* this is the best way to detect internal aimbots, nobody does this. */
// fully functional

bool AC_Funcs::possible_aimbot_zero_mouse(c_usercmd* cmd) {
	if (!cmd)
		return false;

	// only consider while firing (reduces FP a lot)
	const bool firing = (cmd->buttons & (1 << 0)); // IN_ATTACK

	if (!info.prev_cmd) {
		return false;
	}

	const float dYaw = AngleDiff(cmd->viewangles.y, info.prev_cmd->viewangles.y);
	const float dPitch = (cmd->viewangles.x - info.prev_cmd->viewangles.x);

	const int mouseAbs = std::abs(cmd->mousedx) + std::abs(cmd->mousedy);

	// Cooldown to avoid repeated triggers every window
	if (g_zm.coolDown > 0)
		g_zm.coolDown--;

	g_zm.windowTicks++;

	const bool zeroMouse = (mouseAbs <= 1);

	// Thresholds: conservative, tune if needed
	const bool bigAngle = (std::fabs(dYaw) > 1.5f || std::fabs(dPitch) > 1.0f);

	// Count hits only during firing to avoid FP from UI/tab weirdness
	if (firing && zeroMouse && bigAngle)
		g_zm.hits++;

	if (g_zm.windowTicks >= 64) {
		const int hits = g_zm.hits;

		g_zm.windowTicks = 0;
		g_zm.hits = 0;

		const bool suspicious = (hits > 1);

		if (suspicious && g_zm.coolDown == 0) {
			g_zm.coolDown = 128; // 2s cooldown @64tick (or 1s @128)
			return true;
		}
	}

	return false;
}

bool AC_Funcs::possible_desync(bool send_packet, c_usercmd* cmd) {
	if (send_packet)
		info.sent_angle = cmd->viewangles.y;
	else
		info.choked_angle = cmd->viewangles.y;

	bool possible_desync = fabs(fabs(info.sent_angle) - fabs(info.choked_angle)) >= 50.f && info.choked_angle != 0.f && info.sent_angle != 0.f;

	return possible_desync;
}

bool AC_Funcs::possible_backtrack(c_usercmd* cmd) {
	if (info.cmd_tickcount == 0) {
		info.cmd_tickcount = cmd->tick_count;
		return false;
	}

	info.previous_cmd_tickcount = info.cmd_tickcount;
	info.cmd_tickcount = cmd->tick_count;

	if (info.cmd_tickcount < info.previous_cmd_tickcount) {
		int delta = fabs(info.cmd_tickcount - info.previous_cmd_tickcount);
		if (delta > 3 && delta <= 32 && cmd->buttons & 1) // in_attack a.k.a shooting.
			return true; // he had to use backtrack (changing cmd tickcount to enemies previous simulation time and shooting at that position ensures a hit due to how lagcompensation works)
		else
			info.cmd_tickcount = info.previous_cmd_tickcount = 0;
	}

	return false;
}

bool AC_Funcs::possible_rage_aim(c_usercmd* cmd, c_usercmd* prev_cmd) { // more of a ragebot check, not just aimbot because most of the things can be triggered by other form of cheating.
	if (!prev_cmd || !cmd)
		return false;

	// this detects big flicks of rage aimbot, can trigger "false" positives when using jitter antaim, but no way for legit players to trigger these checks even with insane sensitivity and dpi
	// valve uses 25 degree check in casual games, doesnt use it in matchmaking for some reason.

	if (fabs(cmd->viewangles.x - prev_cmd->viewangles.x) >= 60.f) // pitch flick of more than 50 degrees, surely aimbot with pitch antiaim
		return true;

	if(info.last_cmd_number_for_engine_angle != 0 && info.last_engine_angle.valid())
		return (fabs(info.last_engine_angle.x - cmd->viewangles.x) > 5.f || fabs(info.last_engine_angle.y - cmd->viewangles.y) > 5.f);

	// difference between engine angle and the viewangles in sent cmd are not matching by margin greater than .2f (silent aim has been used, or engine view angles (where camera looks) are different than what has been sent in cmd)
	// how silent aim works: cheat sets cmd angles without setting the engine angles (this can be triggered by antiaim too, no way for legit player to trigger)
	// therefore the cheater can still look around like he was not locking on players / anti-aiming.

	return false;
}

std::vector<AC_Funcs::AC_Packet> g_packetBuffer;

bool AC_Funcs::send_ac_batch(const AC_Packet* pkts, uint32_t count)
{
	if (!pkts || count == 0)
		return false;

	// Build payload: [count][packets...]
	const size_t payloadSize = sizeof(uint32_t) + (sizeof(AC_Packet) * (size_t)count);
	std::vector<uint8_t> payload(payloadSize);

	std::memcpy(payload.data(), &count, sizeof(uint32_t));
	std::memcpy(payload.data() + sizeof(uint32_t), pkts, sizeof(AC_Packet) * (size_t)count);

	HINTERNET hSession = WinHttpOpen(
		L"ACClient/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0);

	if (!hSession)
		return false;

	// localhost:80 (XAMPP Apache default)
	HINTERNET hConnect = WinHttpConnect(
		hSession,
		L"localhost",
		INTERNET_DEFAULT_HTTP_PORT,
		0);

	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return false;
	}

	// POST /ac_batch.php
	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect,
		L"POST",
		L"/ac_batch.php",
		NULL,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		0); // HTTP => no WINHTTP_FLAG_SECURE

	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	// Optional: timeouts (good idea in-game)
	WinHttpSetTimeouts(hRequest, 1500, 1500, 1500, 1500);

	BOOL ok = WinHttpSendRequest(
		hRequest,
		L"Content-Type: application/octet-stream\r\n",
		-1L,
		payload.data(),
		(DWORD)payload.size(),
		(DWORD)payload.size(),
		0);

	if (ok)
		ok = WinHttpReceiveResponse(hRequest, NULL);

	// (Optional) Read response body if you want, but not needed for fire-and-forget.

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return ok == TRUE;
}

struct AC_BatchData {
	std::vector<AC_Funcs::AC_Packet> packets;
};

DWORD WINAPI ac_send_batch_thread(LPVOID param)
{
	AC_BatchData* data = (AC_BatchData*)param;
	if (data && !data->packets.empty())
	{
		send_ac_batch(data->packets.data(), (uint32_t)data->packets.size());
	}

	delete data;
	return 0;
}

void ac_async(std::vector<AC_Funcs::AC_Packet>& buffer)
{
	if (buffer.empty())
		return;

	// copy to heap
	AC_BatchData* data = new AC_BatchData();
	data->packets.swap(buffer);

	HANDLE hThread = CreateThread(nullptr, 0, ac_send_batch_thread, data, 0, nullptr);
	if (hThread)
		CloseHandle(hThread);
	else
		delete data;
}

bool AC_Funcs::shifting() {
	if (!hooks::local_player)
		return false;

	float current_sim = hooks::local_player->simulation_time();
	float old_sim = hooks::local_player->old_simulation_time();
	int delta = time_to_ticks(fabs(current_sim - old_sim));
	if (current_sim < old_sim)
		if (delta > 6) // lowest shift in matchmaking is 6 ticks for hideshots (hides your on-shot flick since antiaim cant run at that point)
			return true;
		else
			return false;
	else
		return false;
}

void AC_Funcs::determine_ban_createmove(bool send_packet, c_usercmd* cmd) {

	hooks::get_local_player();

	if (!hooks::local_player || hooks::local_player->simulation_time() == 0.f)
		return;

	AC_Packet pkt{};
	player_info_t player_info;
	interfaces::engine->get_player_info(hooks::local_player->index(), &player_info);
	strncpy_s(pkt.session_id, g_sessionId.c_str(), _TRUNCATE);
	pkt.steam_id = player_info.xuid; //steamid
	pkt.tick = interfaces::clientstate->m_iServerTick;
	pkt.flags = AC_NONE;

	if (possible_desync(send_packet, cmd)) 
		pkt.flags |= AC_DESYNC_POSSIBLE;
	
	if (using_fakelag()) 
		pkt.flags |= AC_FAKELAG_PATTERN;	

	if (fabs(cmd->viewangles.z) >= 30.f || (info.prev_cmd && fabs(info.prev_cmd->viewangles.z) >= 30.f))  // Z angle / roll angle is not networked to clients, only to server
		pkt.flags |= AC_ROLL_ANGLE;	

	if (possible_backtrack(cmd)) 
		pkt.flags |= AC_BACKTRACK;	

	if (cmd->buttons & (1 << 22)) //dont need a better check to be honest, bullrush is more than enough for this.
		pkt.flags |= AC_FAKEDUCK;	

	if(using_micromovement(cmd))
		pkt.flags |= AC_MICROMOVE_PATTERN;

	if(possible_aimbot_zero_mouse(cmd)) // decent even against legit aimbots / rcs
		pkt.flags |= AC_ZERO_MOUSE;

	if (cmd && info.cmd_tickcount && info.prev_cmd)
		if (info.prev_cmd->command_number)
			if (possible_rage_aim(cmd, info.prev_cmd))
				pkt.flags |= AC_RAGE_AIM;

	if (shifting()) 
		pkt.flags |= AC_TICKBASE_SHIFT;

	std::cout << sizeof(AC_Packet) << std::endl;

	g_packetBuffer.push_back(pkt);

	if (g_packetBuffer.size() >= 512) { // every 512 ticks = 8 seconds
		ac_async(g_packetBuffer);
	}
	info.prev_cmd = cmd;
}