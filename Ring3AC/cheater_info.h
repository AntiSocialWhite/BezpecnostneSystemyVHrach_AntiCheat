#pragma once
#include <iostream>
#include <stdlib.h>
#include <cstdlib>
#include <map>
#include <string>
#include <windows.h>
#include <wtsapi32.h>
#include <winuser.h>
#include <thread>
#include <psapi.h>
#include <vector>
#include <tlhelp32.h>
#include <fstream>
#pragma comment(lib,"WtsApi32")
#include <softpub.h>
#include <wintrust.h>
#include <wincrypt.h>
#pragma comment(lib, "wintrust")
#include <comdef.h>
#include <Wbemidl.h>
#include <intrin.h>
#include <tchar.h>
#include <cwchar> 
#include <winreg.h>
#include <Iphlpapi.h>
#include <sstream>
#include <winternl.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Iphlpapi.lib")

#ifndef TH32CS_PROCESS
#define TH32CS_PROCESS 0x00000002
#endif

namespace Handler {
	bool TerminateProcessByPID(DWORD processID);
	void cheat_hndl();
}

namespace SInput {
	void run_sendinput_check();
}

namespace BWriteDetection {
	void InstallHook();
}

namespace LLInjection {
	void InstallHook();
}

namespace DllModuleInjection {
	void Run();
}

namespace ac_shared
{
    constexpr const char* kMappingName = "Local\\ACSessionBridge";
    constexpr DWORD kMagic = 0x41435342; // 'ACSB'
    constexpr size_t kSessionLength = 128;

    struct SharedSessionBlock
    {
        DWORD magic;
        DWORD game_pid;
        LONG ready;
        char session_id[kSessionLength];
    };

    class SessionBridge
    {
    public:
        SessionBridge();
        ~SessionBridge();

        SessionBridge(const SessionBridge&) = delete;
        SessionBridge& operator=(const SessionBridge&) = delete;

        bool CreateForMain(DWORD gamePid, const std::string& sessionId);
        bool OpenForDll();
        void Close();

        bool WriteSession(DWORD gamePid, const std::string& sessionId);
        std::string ReadSession(DWORD expectedPid) const;

        bool IsOpen() const;
        SharedSessionBlock* GetBlock() const;

    private:
        HANDLE m_mapping;
        SharedSessionBlock* m_block;
    };
}

namespace anti_debug
{
    bool IsDebuggerPresentWinAPI();
    bool CheckRemoteDebugger();
    bool CheckPEBBeingDebugged();
    bool CheckDebugPort();
    bool CheckDebugFlags();
    bool CheckDebugObjectHandle();

    bool IsAnyDebuggerPresent();
}

namespace VmDetection {
    const std::string vmMACPrefixes[] = {
        // VMware
        "00:05:69",
        "00:0C:29",
        "00:50:56",
        "00:1C:14",
        "00:1A:4D",
        "00:0C:41",

        // VirtualBox
        "08:00:27",
        "0A:00:27",
        "08:00:00",

        // Hyper-V
        "00:15:5D",
        "00:03:FF",

        // Xen
        "00:16:3E",

        // Parallels
        "00:1F:16",
        "00:1C:42",

        // QEMU/KVM
        "52:54:00",
        "56:00:00",

        // Citrix XenServer
        "00:0C:29",

        // OpenVZ
        "00:1E:67",
        "46:00:00",

        // Nutanix
        "00:02:5A",
        "E4:11:5B",

        // Amazon EC2
        "02:0E:13",
        "02:00:4D",

        // Google Cloud Platform
        "42:00:00"
    };

	void IsRunningInVM();
}

class cheater_info_ {
public:
	DWORD PID;
	int injected_input_detection;
	int memory_write_to_game;
	bool should_quit_game;
};

inline cheater_info_ info;
extern ac_shared::SessionBridge g_bridge;
extern std::string g_sessionId;