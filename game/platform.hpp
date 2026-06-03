//
// Platform / host layer — owns the OS window and the graphics device.
//
// These two are lifetime-linked (the device is created from the window handle),
// so they live together here instead of cluttering Game with platform concerns.
// This is also the Metal seam: a future MacPlatform owns an NSWindow + a Metal
// GraphicsDevice, and Game is unaffected because it only talks to the accessors.
//
#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include "platforms/display_win32.hpp"
#include "rhi/graphics_device.hpp"

namespace val_cg {

    class Platform {
    public:
        Platform(const std::string& windowName, int width, int height)
            : width(width), height(height), display(windowName, width, height)
        {
            device = rhi::CreateGraphicsDevice(display.GetWindowHandle(), width, height);
        }

        rhi::GraphicsDevice* Device()       const { return device.get(); }
        rhi::CommandList*    CommandList()   const { return device->GetCommandList(); }
        HWND                 WindowHandle()  const { return display.GetWindowHandle(); }
        DisplayWin32&        Display()             { return display; }
        int Width()  const { return width; }
        int Height() const { return height; }

    private:
        int width;
        int height;
        DisplayWin32 display;                          // declared before device — constructed first
        std::unique_ptr<rhi::GraphicsDevice> device;
    };

}
