//
// Created by Val on 26.02.2026.
//
// Win32 window + message pump + raw input. The ONLY place (besides rhi/d3d11)
// that includes <windows.h>.
//
#pragma once

#include <windows.h>
#include <string>

#include "display.hpp"
#include "InputDevice.h"

namespace val_cg {

    class DisplayWin32 : public Display {
    public:
        DisplayWin32(std::string name, int clientWidth, int clientHeight);

        void* NativeHandle() const override { return hWnd; }
        bool  PumpEvents() override;
        void  SetTitle(const char* title) override;
        bool  CursorClientPos(int& x, int& y) const override;
        void  SetInputDevice(InputDevice* input) override { mInputDevice = input; }
        int   Width()  const override { return ClientWidth; }
        int   Height() const override { return ClientHeight; }

        HWND GetWindowHandle() const { return hWnd; }

        // Raw-input sink used by the static WndProc.
        static InputDevice* mInputDevice;

    private:
        void RegisterRawInput() const;

        int ClientWidth;
        int ClientHeight;
        HINSTANCE hInstance;
        HWND hWnd{};
        WNDCLASSEX wc{};
        HMODULE Module = nullptr;

        std::string Name;
    };

}
