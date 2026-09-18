//
// Display — the OS window, abstracted.
//
// Everything Game needs from the windowing system goes through this interface so
// that Game/Platform stay free of Win32/AppKit. Concrete implementations live in
// game/platforms/display_win32.* (Win32) and, later, display_mac.mm (AppKit).
//
// Input: the display translates native events (raw input / NSEvent) into the
// platform-neutral InputDevice callbacks (OnKeyDown / OnMouseMove, Keys = VK codes).
//
#pragma once
#include <memory>
#include <string>

namespace val_cg {
    class InputDevice;

    class Display {
    public:
        virtual ~Display() = default;

        // HWND on Windows, NSWindow* on macOS — handed straight to rhi::CreateGraphicsDevice.
        virtual void* NativeHandle() const = 0;

        // Drain the OS event queue. Returns false once the OS asked us to quit.
        virtual bool PumpEvents() = 0;

        virtual void SetTitle(const char* title) = 0;

        // Cursor position in client (window content) pixels, origin top-left.
        // Returns false if unavailable (cursor outside / no focus).
        virtual bool CursorClientPos(int& x, int& y) const = 0;

        // Where translated input events are delivered. May be null (events dropped).
        virtual void SetInputDevice(InputDevice* input) = 0;

        virtual int Width()  const = 0;
        virtual int Height() const = 0;
    };

    // Platform factory — the windowing counterpart of rhi::CreateGraphicsDevice.
    std::unique_ptr<Display> CreateDisplay(const std::string& name, int clientWidth, int clientHeight);
}
