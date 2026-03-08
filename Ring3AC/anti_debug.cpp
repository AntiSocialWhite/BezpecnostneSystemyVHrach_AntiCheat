#include "cheater_info.h"

namespace anti_debug
{
    using NtQueryInformationProcess_t = NTSTATUS(NTAPI*)(
        HANDLE,
        PROCESSINFOCLASS,
        PVOID,
        ULONG,
        PULONG
        );

    static NtQueryInformationProcess_t GetNtQueryInformationProcess()
    {
        static HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (!ntdll)
            return nullptr;

        return reinterpret_cast<NtQueryInformationProcess_t>(
            GetProcAddress(ntdll, "NtQueryInformationProcess")
            );
    }

    bool IsDebuggerPresentWinAPI()
    {
        return ::IsDebuggerPresent() == TRUE;
    }

    bool CheckRemoteDebugger()
    {
        BOOL debuggerPresent = FALSE;

        if (!::CheckRemoteDebuggerPresent(::GetCurrentProcess(), &debuggerPresent))
            return false;

        return debuggerPresent == TRUE;
    }

    bool CheckPEBBeingDebugged()
    {
#if defined(_M_IX86)
        const PPEB peb = reinterpret_cast<PPEB>(__readfsdword(0x30));
#elif defined(_M_X64)
        const PPEB peb = reinterpret_cast<PPEB>(__readgsqword(0x60));
#else
        return false;
#endif

        if (!peb)
            return false;

        return peb->BeingDebugged != 0;
    }

    bool CheckDebugPort()
    {
        auto NtQueryInformationProcess = GetNtQueryInformationProcess();
        if (!NtQueryInformationProcess)
            return false;

        ULONG_PTR debugPort = 0;

        const NTSTATUS status = NtQueryInformationProcess(
            GetCurrentProcess(),
            static_cast<PROCESSINFOCLASS>(7), // ProcessDebugPort
            &debugPort,
            static_cast<ULONG>(sizeof(debugPort)),
            nullptr
        );

        if (status != 0)
            return false;

        return debugPort != 0;
    }

    bool CheckDebugFlags()
    {
        auto NtQueryInformationProcess = GetNtQueryInformationProcess();
        if (!NtQueryInformationProcess)
            return false;

        ULONG debugFlags = 0;

        const NTSTATUS status = NtQueryInformationProcess(
            GetCurrentProcess(),
            static_cast<PROCESSINFOCLASS>(31), // ProcessDebugFlags
            &debugFlags,
            static_cast<ULONG>(sizeof(debugFlags)),
            nullptr
        );

        if (status != 0)
            return false;

        // Usually:
        // not debugged -> nonzero
        // debugged     -> 0
        return debugFlags == 0;
    }

    bool CheckDebugObjectHandle()
    {
        auto NtQueryInformationProcess = GetNtQueryInformationProcess();
        if (!NtQueryInformationProcess)
            return false;

        HANDLE debugObject = nullptr;

        const NTSTATUS status = NtQueryInformationProcess(
            GetCurrentProcess(),
            static_cast<PROCESSINFOCLASS>(30), // ProcessDebugObjectHandle
            &debugObject,
            static_cast<ULONG>(sizeof(debugObject)),
            nullptr
        );

        if (status != 0)
            return false;

        return debugObject != nullptr;
    }

    bool IsAnyDebuggerPresent()
    {
        return
            IsDebuggerPresentWinAPI() ||
            CheckRemoteDebugger() ||
            CheckPEBBeingDebugged() ||
            CheckDebugPort() ||
            CheckDebugFlags() ||
            CheckDebugObjectHandle();
    }
}