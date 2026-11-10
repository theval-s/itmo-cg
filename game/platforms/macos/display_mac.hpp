//
// Created by Volkov Sergey on 23/09/2026.
//
// AppKit window + event pump + input translation, in plain C++ on top of metal-cpp
// (+ appkit_ext.hpp for what the AppKit extension lacks). The macOS counterpart of
// display_win32.*; the ONLY place (besides rhi/metal) that touches AppKit.
//
#pragma once

#include <string>
#include <vector>

#include "platforms/display.hpp"
#include "Keys.h"

namespace CA { class MetalLayer; }
namespace val_cg::appkit { class Window; class Event; }

namespace val_cg {

    class DisplayMac final : public Display {
    public:
        DisplayMac(const std::string& name, int clientWidth, int clientHeight);
        ~DisplayMac() override;

        // The content view's CA::MetalLayer* (not the NSWindow): it is all the Metal device
        // needs (setDevice / nextDrawable / drawableSize).
        void* NativeHandle() const override { return mLayer; }
        bool  PumpEvents() override;
        void  SetTitle(const char* title) override;
        bool  CursorClientPos(int& x, int& y) const override;
        void  SetInputDevice(InputDevice* input) override { mInputDevice = input; }
        int   Width()  const override { return ClientWidth; }
        int   Height() const override { return ClientHeight; }

        // Called by the runtime-made delegate object (display_mac.cpp).
        void OnQuitRequested() { mQuitRequested = true; }
        void OnFocusLost();

    private:
        // Returns true if the event was consumed and must not reach NSApp sendEvent.
        bool HandleEvent(appkit::Event* event);
        void HandleKey(appkit::Event* event, bool down);
        void HandleModifiers(appkit::Event* event);
        void HandleMouse(appkit::Event* event);

        void SendKey(Keys key, bool down);
        void SendMouse(int buttonFlags, int wheelDelta, int dx, int dy);
        // Window coords (bottom-left origin) → client pixels (top-left origin).
        void WindowToClient(double wx, double wy, int& x, int& y) const;

        int ClientWidth;
        int ClientHeight;

        appkit::Window* mWindow   = nullptr;  // owned (+1), released in the destructor
        CA::MetalLayer* mLayer    = nullptr;  // owned (+1)
        void*           mDelegate = nullptr;  // owned (+1); app + window delegate, see display_mac.cpp

        InputDevice* mInputDevice = nullptr;
        bool mQuitRequested = false;
        unsigned long mPrevModifierFlags = 0;
        double mMouseRemX = 0, mMouseRemY = 0;  // sub-pixel motion carried to the next event
        std::vector<Keys> mHeldKeys;          // released on focus loss so nothing sticks
    };

}
