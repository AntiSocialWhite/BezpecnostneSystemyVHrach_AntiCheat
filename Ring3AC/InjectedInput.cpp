#include "cheater_info.h"5

// Global hook handle for the mouse hook
HHOOK hMouseHook;

// Low-level mouse hook procedure
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Check if the message is valid and should be processed
    if (nCode == HC_ACTION)
    {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;

        // Check if the input event was injected
        if (pMouse->flags & LLMHF_INJECTED)
        {
            info.injected_input_detection++; // Found injected input for mouse (no recoil / aimbot - SendInput)
        }
    }

    // Call the next hook in the chain
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

// Function to set up the mouse hook
void SetMouseHook()
{
    // Install the low-level mouse hook
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    if (hMouseHook == NULL)
    {
        std::cerr << "Failed to install mouse hook!" << std::endl;
    }
    else
    {
        std::cout << "Mouse hook installed successfully." << std::endl;
    }
}

// Function to remove the mouse hook
void RemoveMouseHook()
{
    UnhookWindowsHookEx(hMouseHook);
    std::cout << "Mouse hook removed." << std::endl;
}

void SInput::run_sendinput_check() {
    // Set the global mouse hook
    SetMouseHook();

    // Message loop to keep the application running
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Remove the hook before exiting
    RemoveMouseHook();
}