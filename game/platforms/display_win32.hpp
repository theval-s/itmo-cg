//
// Created by Val on 26.02.2026.
//

#pragma once

#include <windows.h>
#include <string>



class DisplayWin32 {
private:
    int ClientWidth;
    int ClientHeight;
    HINSTANCE hInstance;
    HWND hWnd{};
    WNDCLASSEX wc{};
    HMODULE Module = nullptr;

    std::string Name;

public:
    DisplayWin32(std::string name, int clientWidth, int clientHeight);
    HWND GetWindowHandle() const {
        return hWnd;
    }
};
