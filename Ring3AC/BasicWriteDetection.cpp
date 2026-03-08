#include "cheater_info.h"

// Function pointer to store the original WriteProcessMemory function
typedef BOOL(WINAPI* WriteProcessMemory_t)(
    HANDLE hProcess,
    LPVOID lpBaseAddress,
    LPCVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T* lpNumberOfBytesWritten);

WriteProcessMemory_t OriginalWriteProcessMemory = nullptr;

std::string GetProcessName(DWORD processID) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hProcess) {
        char processName[MAX_PATH] = "";
        HMODULE hMod;
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleFileNameExA(hProcess, hMod, processName, sizeof(processName) / sizeof(char));
        }
        CloseHandle(hProcess);
        return std::string(processName);
    }
    return "";
}

BOOL WINAPI HookedWriteProcessMemory(
    HANDLE hProcess,
    LPVOID lpBaseAddress,
    LPCVOID lpBuffer,
    SIZE_T nSize,
    SIZE_T* lpNumberOfBytesWritten) {

    // Get the process ID
    DWORD processID = GetProcessId(hProcess);

    // Check if the process name is csgo.exe
    std::string processName = GetProcessName(processID);
    if (processName == "csgo.exe") {
        // Log the details
        std::cout << "Detected WriteProcessMemory call to csgo.exe!" << std::endl;
        std::cout << "Process ID: " << processID << std::endl;
        std::cout << "Base Address: " << lpBaseAddress << std::endl;
        std::cout << "Buffer Size: " << nSize << std::endl;

        return FALSE; // Block the write
    }

    // Call the original function after logging
    return OriginalWriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

void BWriteDetection::InstallHook() {
    // Detour the WriteProcessMemory API call to our HookedWriteProcessMemory function
    OriginalWriteProcessMemory = (WriteProcessMemory_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "WriteProcessMemory");

    if (OriginalWriteProcessMemory) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)OriginalWriteProcessMemory, HookedWriteProcessMemory);
        DetourTransactionCommit();
        std::cout << "Hook installed." << std::endl;
    }
}
