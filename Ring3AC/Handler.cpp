#include "cheater_info.h"
#include <windows.h>
#include <winhttp.h>
#include <string>

bool Handler::TerminateProcessByPID(DWORD processID) {
    // Open the process with the desired access rights (PROCESS_TERMINATE)
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    // Attempt to terminate the process
    if (!TerminateProcess(hProcess, 0)) {
        std::cerr << "Failed to terminate process. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Close the process handle
    CloseHandle(hProcess);
    std::cout << "Process with PID " << processID << " terminated successfully." << std::endl;
    return true;
}
#pragma comment(lib, "winhttp.lib")

#include <windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

bool IsSessionBanned(const std::wstring& host, const std::wstring& endpoint)
{
    bool banned = false;

    HINTERNET hSession = WinHttpOpen(
        L"ACBanCheck/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        host.c_str(),
        INTERNET_DEFAULT_HTTP_PORT,
        0);

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        endpoint.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0))
    {
        if (WinHttpReceiveResponse(hRequest, nullptr))
        {
            char buffer[512] = {};
            DWORD bytesRead = 0;

            if (WinHttpReadData(
                hRequest,
                buffer,
                sizeof(buffer) - 1,
                &bytesRead))
            {
                buffer[bytesRead] = 0;
                std::string response(buffer);

                if (response.find("\"banned\":true") != std::string::npos)
                    banned = true;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return banned;
}

bool CheckDLLAlive(const std::wstring& host, const std::wstring& endpoint)
{
    bool result = false;
#ifdef _DEBUG
    std::cout << "Handler session before check: [" << g_sessionId << "]" << std::endl;
#endif

    HINTERNET hSession = WinHttpOpen(L"ACWatchdog/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(),
        INTERNET_DEFAULT_HTTP_PORT, 0);

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        endpoint.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    if (WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0))
    {
        if (WinHttpReceiveResponse(hRequest, NULL))
        {
            char buffer[128];
            DWORD bytesRead;

            if (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead))
            {
                buffer[bytesRead] = 0;

                std::string response(buffer);

                if (response.find("OK") != std::string::npos)
                    result = true;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

std::wstring BuildSessionPath(const std::string& path,const std::string& session)
{
    std::wstring ws(session.begin(), session.end());
    std::wstring ws1(path.begin(), path.end());
    return L"" + ws1 + ws;
}

bool ProcessSessionOnServer(const std::string& sessionId)
{
    HINTERNET hSession = WinHttpOpen(
        L"ACSessionWorker/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"localhost",
        INTERNET_DEFAULT_HTTP_PORT,
        0
    );

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring ws(sessionId.begin(), sessionId.end());
    std::wstring path = L"/api/process_session.php?session=" + ws;

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

    if (WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0))
    {
        if (WinHttpReceiveResponse(hRequest, nullptr))
        {
            DWORD statusCode = 0;
            DWORD statusSize = sizeof(statusCode);

            if (WinHttpQueryHeaders(
                hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode,
                &statusSize,
                WINHTTP_NO_HEADER_INDEX))
            {
                ok = (statusCode == 200);
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return ok;
}


void Handler::cheat_hndl() {
    Sleep(20000); // to wait for first dll heartbeat.

    while (true) {

        ProcessSessionOnServer(g_sessionId);

        std::wstring endpoint_dll = BuildSessionPath("/api/check_dll.php?session=", g_sessionId);
        std::wstring endpoint_ban = BuildSessionPath("/api/check_ban.php?session=", g_sessionId);

        bool dllAlive = CheckDLLAlive(L"localhost", endpoint_dll);
        bool banned = IsSessionBanned(L"localhost", endpoint_ban);
#ifdef _DEBUG
        std::cout << "Handler session before check: [" << g_sessionId << "]\n";
        std::cout << "dllAlive = " << dllAlive << "\n";
        std::cout << "banned   = " << banned << "\n";
#endif

        if (!dllAlive) {
            std::cout << "Quit reason: DLL not alive\n";
            info.should_quit_game = true;
        }

        if (banned) {
            std::cout << "Quit reason: Session banned\n";
            info.should_quit_game = true;
        }

        if (info.injected_input_detection)
        {
            if (info.injected_input_detection >= 5) {
                info.should_quit_game = true;
            }
        }

        if (info.should_quit_game)
        {
            TerminateProcessByPID(info.PID);
            info.should_quit_game = false;
            info.PID = 0;
            info.injected_input_detection = 0;
        }

        Sleep(10000); // to wait for dll heartbeat.
    }
}