//
// Created by Val on 26.02.2026.
//

#include <iostream>

#include "display_win32.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
        {
            // If a key is pressed send it to the input object so it can record that state.
            //std::cout << "Key: " << static_cast<unsigned int>(wParam) << std::endl;
            //PostMessage(hWnd, WM_KEYDOWN, wParam, lParam);
            if (static_cast<unsigned int>(wParam) == 27) PostQuitMessage(0);
            return 0;
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        default:
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
}

DisplayWin32::DisplayWin32(std::string name, int clientWidth, int clientHeight) {
    Name = name;
    hInstance = GetModuleHandle(nullptr);

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_SHIELD);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = Name.c_str();
    wc.cbSize = sizeof(WNDCLASSEX);

    RegisterClassEx(&wc);

    ClientHeight = clientHeight;
    ClientWidth = clientWidth;

    RECT wRect = {0,0, static_cast<LONG>(ClientWidth), static_cast<LONG>(ClientHeight)};
    AdjustWindowRect(&wRect, WS_OVERLAPPEDWINDOW, FALSE);
    auto posX = (GetSystemMetrics(SM_CXSCREEN) - ClientWidth)/2;
    auto posY = (GetSystemMetrics(SM_CYSCREEN) - ClientHeight)/2;

    auto dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
    hWnd = CreateWindowEx(WS_EX_APPWINDOW, Name.c_str(), Name.c_str(), dwStyle,
        posX, posY, wRect.right - wRect.left, wRect.bottom - wRect.top,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    ShowCursor(true);
}
