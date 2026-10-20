//
// Created by Val on 26.02.2026.
//

#include <iostream>

#include "display_win32.hpp"

namespace val_cg {

InputDevice* DisplayWin32::mInputDevice = nullptr;

namespace {

// Raw mouse input in absolute mode (RDP, tablets) reports [0,65535] over the
// virtual screen; we turn it into a delta ourselves so InputDevice only ever
// sees relative motion.
int lastAbsX = 0, lastAbsY = 0;
bool haveLastAbs = false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INPUT:
        {
            if (!DisplayWin32::mInputDevice) return 0;
            UINT dwSize = 0;
            GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = new BYTE[dwSize];
            if (lpb == nullptr) {
                return 0;
            }

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
                OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

            RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(lpb);

            if (raw->header.dwType == RIM_TYPEKEYBOARD)
            {
                DisplayWin32::mInputDevice->OnKeyDown({
                    raw->data.keyboard.MakeCode,
                    raw->data.keyboard.Flags,
                    raw->data.keyboard.VKey,
                    raw->data.keyboard.Message
                });
            }
            else if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                int dx = raw->data.mouse.lLastX;
                int dy = raw->data.mouse.lLastY;
                if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                    const int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                    const int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
                    const int absX = static_cast<int>((raw->data.mouse.lLastX / 65535.0f) * screenW);
                    const int absY = static_cast<int>((raw->data.mouse.lLastY / 65535.0f) * screenH);
                    dx = haveLastAbs ? absX - lastAbsX : 0;
                    dy = haveLastAbs ? absY - lastAbsY : 0;
                    lastAbsX = absX; lastAbsY = absY; haveLastAbs = true;
                }

                POINT p;
                GetCursorPos(&p);
                ScreenToClient(hWnd, &p);

                DisplayWin32::mInputDevice->OnMouseMove({
                    raw->data.mouse.usButtonFlags,
                    static_cast<short>(raw->data.mouse.usButtonData),
                    dx,
                    dy,
                    static_cast<int>(p.x),
                    static_cast<int>(p.y)
                });
            }

            delete[] lpb;
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        case WM_KEYDOWN:
        {
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

} // namespace

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

    RegisterRawInput();
}

void DisplayWin32::RegisterRawInput() const {
    RAWINPUTDEVICE Rid[2];

    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x02;
    Rid[0].dwFlags = 0;   // adds HID mouse and also ignores legacy mouse messages
    Rid[0].hwndTarget = hWnd;

    Rid[1].usUsagePage = 0x01;
    Rid[1].usUsage = 0x06;
    Rid[1].dwFlags = 0;   // adds HID keyboard and also ignores legacy keyboard messages
    Rid[1].hwndTarget = hWnd;

    if (RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE)
    {
        auto errorCode = GetLastError();
        std::cout << "ERROR: RegisterRawInputDevices failed: " << errorCode << std::endl;
    }
}

bool DisplayWin32::PumpEvents() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

void DisplayWin32::SetTitle(const char* title) {
    SetWindowTextA(hWnd, title);
}

bool DisplayWin32::CursorClientPos(int& x, int& y) const {
    POINT p;
    if (!GetCursorPos(&p)) return false;
    if (!ScreenToClient(hWnd, &p)) return false;
    x = p.x;
    y = p.y;
    return true;
}

std::unique_ptr<Display> CreateDisplay(const std::string& name, int clientWidth, int clientHeight) {
    return std::make_unique<DisplayWin32>(name, clientWidth, clientHeight);
}

} // namespace val_cg
