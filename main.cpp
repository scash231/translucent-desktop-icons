#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

// Struct to store target handles during iteration
struct FindDefViewData {
    HWND hwndDefView = NULL;
    HWND hwndWorkerW = NULL;
};

// Callback to search for SHELLDLL_DefView inside WorkerW windows
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    FindDefViewData* data = reinterpret_cast<FindDefViewData*>(lParam);
    
    char className[256];
    if (GetClassNameA(hwnd, className, 256) && strcmp(className, "WorkerW") == 0) {
        HWND defView = FindWindowExA(hwnd, NULL, "SHELLDLL_DefView", NULL);
        if (defView != NULL) {
            data->hwndDefView = defView;
            data->hwndWorkerW = hwnd;
            return FALSE; // Stop enumerating
        }
    }
    return TRUE; // Continue enumerating
}

struct DesktopHandles {
    HWND progman = NULL;
    HWND workerW = NULL;
    HWND defView = NULL;
    HWND listView = NULL;
};

DesktopHandles GetDesktopHandles() {
    DesktopHandles handles;
    
    // 1. Try finding it under Progman
    handles.progman = FindWindowA("Progman", NULL);
    if (handles.progman != NULL) {
        handles.defView = FindWindowExA(handles.progman, NULL, "SHELLDLL_DefView", NULL);
    }
    
    // 2. If not found, SHELLDLL_DefView might be nested inside a WorkerW window.
    if (handles.defView == NULL) {
        FindDefViewData data;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
        handles.defView = data.hwndDefView;
        handles.workerW = data.hwndWorkerW;
    }
    
    // 3. Find the SysListView32 child control, which holds the desktop icons.
    if (handles.defView != NULL) {
        handles.listView = FindWindowExA(handles.defView, NULL, "SysListView32", NULL);
    }
    
    return handles;
}

bool SetDesktopIconOpacity(int opacity, const std::string& targetLayer) {
    if (opacity < 0 || opacity > 255) {
        std::cerr << "Error: Opacity must be between 0 and 255." << std::endl;
        return false;
    }
    
    DesktopHandles handles = GetDesktopHandles();
    HWND hwndTarget = NULL;
    std::string layerName = "";
    
    if (targetLayer == "defview") {
        hwndTarget = handles.defView;
        layerName = "SHELLDLL_DefView";
    } else if (targetLayer == "workerw") {
        hwndTarget = (handles.workerW != NULL) ? handles.workerW : handles.progman;
        layerName = (handles.workerW != NULL) ? "WorkerW" : "Progman";
    } else {
        hwndTarget = handles.listView;
        layerName = "SysListView32";
    }
    
    if (hwndTarget == NULL) {
        std::cerr << "Error: Could not find handle for target layer: " << layerName << std::endl;
        return false;
    }
    
    std::cout << "Targeting layer: " << layerName << " (handle: 0x" << std::hex << reinterpret_cast<void*>(hwndTarget) << std::dec << ")" << std::endl;
    
    // Query current extended style
    LONG_PTR style = GetWindowLongPtrA(hwndTarget, GWL_EXSTYLE);
    
    if (opacity == 255) {
        // Revert layering if opacity is 100% (255) to avoid extra composition overhead.
        if (style & WS_EX_LAYERED) {
            SetWindowLongPtrA(hwndTarget, GWL_EXSTYLE, style & ~WS_EX_LAYERED);
            SetWindowPos(hwndTarget, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            std::cout << "Removed WS_EX_LAYERED style (fully opaque)." << std::endl;
        } else {
            std::cout << "Layer is already fully opaque." << std::endl;
        }
    } else {
        // Enable layering if not already enabled.
        if (!(style & WS_EX_LAYERED)) {
            SetWindowLongPtrA(hwndTarget, GWL_EXSTYLE, style | WS_EX_LAYERED);
            SetWindowPos(hwndTarget, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            std::cout << "Enabled WS_EX_LAYERED style." << std::endl;
        }
        
        if (SetLayeredWindowAttributes(hwndTarget, 0, static_cast<BYTE>(opacity), LWA_ALPHA)) {
            std::cout << "Successfully set opacity to " << opacity << " (" 
                       << (opacity * 100 / 255) << "%)." << std::endl;
        } else {
            DWORD error = GetLastError();
            std::cerr << "Error: SetLayeredWindowAttributes failed with code " << error << "." << std::endl;
            return false;
        }
    }
    
    // Redraw target window and parent windows to refresh composition layout.
    RedrawWindow(hwndTarget, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    HWND parent = GetParent(hwndTarget);
    while (parent != NULL) {
        RedrawWindow(parent, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        parent = GetParent(parent);
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "===========================================\n";
        std::cout << "   Desktop Icon Opacity Manipulator Tool   \n";
        std::cout << "===========================================\n";
        std::cout << "Running in Interactive Mode.\n";
        std::cout << "Usage with CLI: desktop_icons.exe <opacity_0-255> [listview|defview|workerw]\n\n";

        std::string targetLayer = "listview";
        int opacity = -1;

        while (true) {
            std::cout << "\nCurrent target layer: [" << targetLayer << "]\n";
            std::cout << "Commands:\n";
            std::cout << "  0 - 255  : Change desktop opacity to this value\n";
            std::cout << "  t        : Change target layer (listview / defview / workerw)\n";
            std::cout << "  q        : Quit application\n";
            std::cout << "Enter command: ";

            std::string input;
            if (!std::getline(std::cin, input)) {
                break;
            }

            // Trim leading/trailing whitespace
            input.erase(0, input.find_first_not_of(" \t\r\n"));
            input.erase(input.find_last_not_of(" \t\r\n") + 1);

            if (input.empty()) {
                continue;
            }

            if (input == "q" || input == "Q" || input == "exit") {
                break;
            }

            if (input == "t" || input == "T") {
                std::cout << "Enter target layer ('listview', 'defview', 'workerw'): ";
                std::string layerInput;
                std::getline(std::cin, layerInput);
                std::transform(layerInput.begin(), layerInput.end(), layerInput.begin(), ::tolower);
                if (layerInput == "listview" || layerInput == "defview" || layerInput == "workerw") {
                    targetLayer = layerInput;
                } else {
                    std::cout << "Invalid layer name. Keeping current target.\n";
                }
                continue;
            }

            try {
                opacity = std::stoi(input);
                if (opacity >= 0 && opacity <= 255) {
                    SetDesktopIconOpacity(opacity, targetLayer);
                } else {
                    std::cout << "Error: Opacity must be between 0 and 255.\n";
                }
            } catch (...) {
                std::cout << "Error: Invalid command. Use a number (0-255), 't' to target, or 'q' to quit.\n";
            }
        }
        return 0;
    }
    
    std::string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
        std::cout << "Usage: desktop_icons.exe <opacity_value> [target_layer]\n";
        std::cout << "  <opacity_value>: integer from 0 (completely transparent) to 255 (completely opaque)\n";
        std::cout << "  [target_layer]: optional, 'listview' (default), 'defview', or 'workerw'\n";
        return 0;
    }
    
    std::string targetLayer = "listview";
    if (argc >= 3) {
        targetLayer = argv[2];
        std::transform(targetLayer.begin(), targetLayer.end(), targetLayer.begin(), ::tolower);
    }
    
    try {
        int opacity = std::stoi(arg);
        if (opacity < 0 || opacity > 255) {
            std::cerr << "Error: Opacity must be between 0 and 255." << std::endl;
            return 1;
        }
        if (SetDesktopIconOpacity(opacity, targetLayer)) {
            return 0;
        } else {
            return 1;
        }
    } catch (const std::exception&) {
        std::cerr << "Error: Invalid argument. Opacity must be an integer from 0 to 255." << std::endl;
        return 1;
    }
}
