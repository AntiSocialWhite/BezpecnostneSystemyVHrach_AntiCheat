#include "DllModule.h"
#include "..\Ring3AC\cheater_info.h"
#include <Windows.h>
#include <string>
bool g_running = true;

std::string WaitForSessionId(DWORD timeoutMs = 5000)
{
	DWORD start = GetTickCount64();

	while (GetTickCount64() - start < timeoutMs)
	{
		ac_shared::SessionBridge bridge;

		if (bridge.OpenForDll())
		{
			std::string session = bridge.ReadSession(GetCurrentProcessId());
			if (!session.empty())
				return session;
		}

		Sleep(50);
	}

	return "";
}

#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

bool SendHeartbeat(const std::wstring& host, const std::wstring& path) // for "is dll alive?" check - if not and we are in game, we close the game.
{
	HINTERNET hSession = WinHttpOpen(
		L"CSGOModule/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0
	);

	if (!hSession)
		return false;

	HINTERNET hConnect = WinHttpConnect(
		hSession,
		host.c_str(),
		INTERNET_DEFAULT_HTTP_PORT,
		0
	);

	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return false;
	}

	HINTERNET hRequest = WinHttpOpenRequest(
		hConnect,
		L"GET",
		path.c_str(),
		nullptr,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		0
	);

	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	bool ok = false;

	if (WinHttpSendRequest(hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS,
		0,
		WINHTTP_NO_REQUEST_DATA,
		0,
		0,
		0))
	{
		if (WinHttpReceiveResponse(hRequest, nullptr))
			ok = true;
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return ok;
}

std::wstring ToWide(const std::string& s)
{
	if (s.empty())
		return L"";

	int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	if (len <= 0)
		return L"";

	std::wstring out(len - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
	return out;
}

std::wstring BuildHeartbeatPath(const std::string& sessionId)
{
	return L"/api/heartbeat.php?session=" + ToWide(sessionId);
}

__forceinline void RemoveHeaders(HINSTANCE instance) {
	DWORD dwProtect = 0x0;
	auto pImageDOS = reinterpret_cast<PIMAGE_DOS_HEADER>(instance);

	if (pImageDOS->e_magic == IMAGE_DOS_SIGNATURE)
	{
		auto pImageNT = reinterpret_cast<PIMAGE_NT_HEADERS>(pImageDOS + pImageDOS->e_lfanew);
		const auto SizeOfPE = pImageNT->FileHeader.SizeOfOptionalHeader;

		if (pImageNT->Signature == IMAGE_NT_SIGNATURE && SizeOfPE)
		{
			const auto HeaderSize = static_cast<size_t>(SizeOfPE);
			VirtualProtect(reinterpret_cast<LPVOID>(instance), HeaderSize, PAGE_EXECUTE_READWRITE, &dwProtect);
			RtlZeroMemory(reinterpret_cast<LPVOID>(instance), HeaderSize);
			VirtualProtect(reinterpret_cast<LPVOID>(instance), HeaderSize, dwProtect, &dwProtect);
		}
	}
}

DWORD WINAPI HeartbeatThread(LPVOID)
{
	while (g_running)
	{
		if (!AC_Funcs::g_sessionId.empty())
		{
			if (!SendHeartbeat(L"localhost", BuildHeartbeatPath(AC_Funcs::g_sessionId)))
				info.should_quit_game = true;
		}

		Sleep(5000);
	}

	return 0;
}

unsigned long WINAPI initialize(void* instance) {
#ifdef _DEBUG
	AC_Funcs::g_sessionId = "debug_session";
#endif

#ifndef _DEBUG
	AC_Funcs::g_sessionId = WaitForSessionId(5000);

	if (AC_Funcs::g_sessionId.empty())
	{
		// failed to get session
		return 0;
	}
#endif

	while (!GetModuleHandleA("serverbrowser.dll"))
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

	try {
		sigs::initialize();
		interfaces::initialize();
		hooks::initialize();
	}

	catch (const std::runtime_error& error) {
		MessageBoxA(nullptr, error.what(), "csgo_module error!", MB_OK | MB_ICONERROR);
		FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
	}

	while (!((GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_END) & 0x8000)))
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

	FreeLibraryAndExitThread(static_cast<HMODULE>(instance), 0);
}

unsigned long WINAPI release() {
	return TRUE;
}

std::int32_t WINAPI DllMain(const HMODULE instance [[maybe_unused]], const unsigned long reason, const void* reserved [[maybe_unused]] ) {
	DisableThreadLibraryCalls(instance);

	switch (reason) {
	case DLL_PROCESS_ATTACH: {
		RemoveHeaders(instance);
		if (auto handle = CreateThread(nullptr, NULL, initialize, instance, NULL, nullptr))
			CloseHandle(handle);

#ifndef _DEBUG
		HANDLE hHeartbeat = CreateThread(nullptr, 0, HeartbeatThread, instance, 0, nullptr);
		if (hHeartbeat)
			CloseHandle(hHeartbeat);

#endif // DEBUG

		break;
	}

	case DLL_PROCESS_DETACH: {
		release();
		g_running = false;
		break;
	}
	}

	return true;
}