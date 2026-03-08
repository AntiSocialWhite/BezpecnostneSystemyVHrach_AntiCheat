#include "cheater_info.h"
#include <random>
#include <winhttp.h>

// This anti-cheat uses a lot of things that i have learned about RING 3 (usermode) anti-cheats like VAC
// do wrong. This is more of a "proof of concept" that works well against all common cheats for CS:GO,
// In real world it would need proper obfuscation, IDA "breakers" to make reverse engineering hard,
// and much more.

ac_shared::SessionBridge g_bridge;
std::string g_sessionId;

std::string GenerateSessionId()
{
    static std::mt19937_64 rng{ std::random_device{}() };

    std::stringstream ss;
    ss << std::hex << rng() << rng();
    return ss.str();
}

bool StartGameSession(DWORD gamePid)
{
    g_sessionId = GenerateSessionId();

    if (!g_bridge.CreateForMain(gamePid, g_sessionId))
        return false;

    std::cout << "Session ID: " << g_sessionId << std::endl;

    std::thread inject_modules(DllModuleInjection::Run);
    inject_modules.join();

    Sleep(2000);

    return true;
}

#include <windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

DWORD GetProcessID(const std::wstring& processName) {
    DWORD processID = 0; // Default value if process is not found

    // Create a snapshot of all processes in the system
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_PROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processID; // Return 0 if snapshot creation failed
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    // Iterate through the processes
    if (Process32First(hSnapshot, &processEntry)) {
        do {
            // Compare the process name with the specified name
            if (processName == processEntry.szExeFile) {
                processID = processEntry.th32ProcessID; // Found the process, get the PID
                break; // Exit the loop since we found the process
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }

    CloseHandle(hSnapshot); // Close the snapshot handle
    return processID; // Return the PID (0 if not found)
}

std::map <std::string, DWORD> get_running_processes()
{
    std::map <std::string, DWORD> result;
    WTS_PROCESS_INFOA* pWPIs;
    DWORD                         dwProcessCount;
    if (WTSEnumerateProcessesA(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcessCount))
    {
        for (DWORD n = 0; n < dwProcessCount; n++) result[pWPIs[n].pProcessName] = pWPIs[n].ProcessId;
        WTSFreeMemory(pWPIs);
    }
    return result;
}

bool check_cheatengine(std::string process)
{
    std::string search_keys[5] = { "cheat", "hacker", "Cheat", "Hacker", "ProcessHacker"};

    for(auto search_key : search_keys)
    if (process.find(search_key) != std::string::npos)
        return true;

    return false;
}

void process_check() {
    std::cout << "process check running" << "\n";

    while (true) {
        auto processes = get_running_processes();

        for (auto name_id : processes) {
           //std::cout << name_id.first << "\n";
            if (check_cheatengine(name_id.first))
                info.should_quit_game = true; // cheatengine - quit game
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void get_process_id() {
    std::wstring targetProcessName = L"csgo.exe"; // Name of the process to find, its a csgo build, so csgo.exe.
    DWORD pid = GetProcessID(targetProcessName);

    if (pid != 0) {
        info.PID = pid;
        std::wcout << L"Process ID of " << targetProcessName << L" is: " << pid << std::endl;
    }
}

int main(int argc, char** argv)
{
    while (info.PID == 0) {
        get_process_id();
    }

    if (info.PID != 0) {
        StartGameSession(info.PID);
        std::thread cheats_handler(Handler::cheat_hndl);
        std::thread vm_handle(VmDetection::IsRunningInVM);
        std::thread sendinput_thread(SInput::run_sendinput_check);
        std::thread process_thread(process_check);
        process_thread.join();
        sendinput_thread.join();        
        cheats_handler.join();
        vm_handle.join();
    }

    return 0;
}
