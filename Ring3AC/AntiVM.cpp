#include "cheater_info.h"
#include <iomanip>
#pragma warning(disable : 4996)

// virtual machine checks, probably everything that can be done in terms of user-mode.
// should be pretty decent, and detect the most common virtual machines.

bool CheckVMDrivers() {
    const wchar_t* drivers[] = { L"vmhgfs", L"vmxnet", L"VBoxMouse", L"VBoxGuest", L"hvix64" };
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (scManager) {
        ENUM_SERVICE_STATUS_PROCESS serviceStatus[256];
        DWORD bytesNeeded, serviceCount;
        if (EnumServicesStatusEx(scManager, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER, SERVICE_STATE_ALL,
            (LPBYTE)serviceStatus, sizeof(serviceStatus), &bytesNeeded, &serviceCount, NULL, NULL)) {
            for (DWORD i = 0; i < serviceCount; i++) {
                for (const wchar_t* driver : drivers) {
                    if (wcsstr(serviceStatus[i].lpServiceName, driver)) {
                        CloseServiceHandle(scManager);
                        return true; // VM driver detected
                    }
                }
            }
        }
        CloseServiceHandle(scManager);
    }
    return false; // No VM drivers detected
}

bool IsVMProcessRunning() {
    const wchar_t* vmProcesses[] = { L"vmtoolsd.exe", L"vboxservice.exe", L"vmwaretray.exe", L"vmwareuser.exe" };
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hProcessSnap, &pe32)) {
        do {
            // Compare process names with known VM-related processes
            for (const wchar_t* vmProcess : vmProcesses) {
                if (wcsstr(pe32.szExeFile, vmProcess)) {
                    CloseHandle(hProcessSnap);
                    return true;  // VM process detected
                }
            }
        } while (Process32NextW(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return false;  // No VM process detected
}

bool CheckForVirtualMachine() {
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return false;

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) return false;

    IWbemLocator* pLocator = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLocator);
    if (FAILED(hres)) return false;

    IWbemServices* pServices = NULL;
    hres = pLocator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, NULL, 0, NULL, NULL, &pServices);
    if (FAILED(hres)) {
        pLocator->Release();
        return false;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pServices->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_ComputerSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres)) {
        pServices->Release();
        pLocator->Release();
        return false;
    }

    IWbemClassObject* pClassObj = NULL;
    ULONG uReturn = 0;
    bool isVM = false;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;
        hr = pClassObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
        if (vtProp.bstrVal && (wcscmp(vtProp.bstrVal, L"Microsoft Corporation") == 0 || wcscmp(vtProp.bstrVal, L"VMware, Inc.") == 0)) {
            isVM = true;
        }
        VariantClear(&vtProp);
        pClassObj->Release();
    }

    pEnumerator->Release();
    pServices->Release();
    pLocator->Release();
    CoUninitialize();
    return isVM;
}

bool CheckForHypervisor() {
    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 1);  // CPUID with EAX=1
    return (cpuInfo[2] & (1 << 31)) != 0; // Check if hypervisor bit is set
}

bool CheckVMRegistry() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\BIOS"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        TCHAR buffer[256];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueEx(hKey, TEXT("SystemProductName"), NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            if (_tcsstr(buffer, TEXT("VMware")) || _tcsstr(buffer, TEXT("VirtualBox"))) {
                RegCloseKey(hKey);
                return true; // Virtual machine detected
            }
        }
        RegCloseKey(hKey);
    }
    return false;
}

// Check for VM network adapters
bool CheckVMNetworkAdapters() {
    const wchar_t* vmAdapters[] = { L"VMware", L"VirtualBox", L"vboxnet", L"VMnet" };
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwBufLen = sizeof(adapterInfo);

    if (GetAdaptersInfo(adapterInfo, &dwBufLen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
        while (pAdapterInfo) {
            for (const wchar_t* adapterName : vmAdapters) {
                wchar_t description[256];
                mbstowcs(description, pAdapterInfo->Description, 256);

                if (wcsstr(description, adapterName)) {
                    return true;  // VM network adapter detected
                }
            }
            pAdapterInfo = pAdapterInfo->Next;
        }
    }
    return false;  // No VM network adapters detected
}

bool CheckMACAddress() {
    IP_ADAPTER_INFO adapterInfo[16];  // Buffer for adapter info
    DWORD dwBufLen = sizeof(adapterInfo);  // Size of the buffer

    // Get the adapter information
    if (GetAdaptersInfo(adapterInfo, &dwBufLen) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;  // Pointer to adapter info

        // Iterate through each adapter
        while (pAdapterInfo) {
            // Format MAC address to string
            std::ostringstream macStream{};
            for (int i = 0; i < 6; i++) {
                macStream << std::setw(2) << std::setfill('0') << std::hex << (int)pAdapterInfo->Address[i];
                if (i < 5) {
                    macStream << ":";  // Add colon separator
                }
            }
            std::string macAddress = macStream.str();  // Get the formatted MAC address

            // Check if MAC address matches any VM prefix
            for (const std::string& prefix : VmDetection::vmMACPrefixes) {
                if (macAddress.substr(0, prefix.length()) == prefix) {
                    return true;  // VM MAC address detected
                }
            }

            pAdapterInfo = pAdapterInfo->Next;  // Move to the next adapter
        }
    }

    return false;  // No VM MAC address detected
}

void VmDetection::IsRunningInVM() {
    while (true) {
        if (CheckForHypervisor() || CheckVMNetworkAdapters() || CheckVMRegistry() || IsVMProcessRunning() || CheckVMDrivers() || CheckMACAddress()) {
            Handler::TerminateProcessByPID(info.PID);
            std::cout << "Quit reason: Virtual Machine\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(60)); // No need to do it more often.
    }
}