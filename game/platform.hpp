//
// Platform / host layer — owns the OS window and the graphics device.
//
// These two are lifetime-linked (the device is created from the window handle),
// so they live together here instead of cluttering Game with platform concerns.
// Both halves are picked by their factories (`CreateDisplay`, `rhi::CreateGraphicsDevice`),
// whose implementations CMake selects per platform — this header knows nothing
// about Win32 or AppKit.
//
#pragma once
#include <memory>
#include <string>
#include "platforms/display.hpp"
#include "rhi/graphics_device.hpp"

namespace val_cg {

    class Platform {
    public:
        Platform(const std::string& windowName, int width, int height)
            : width(width), height(height), display(CreateDisplay(windowName, width, height))
        {
            device = rhi::CreateGraphicsDevice(display->NativeHandle(), width, height);
        }

        rhi::GraphicsDevice* Device()       const { return device.get(); }
        rhi::CommandList*    CommandList()   const { return device->GetCommandList(); }
        val_cg::Display&     Display()       const { return *display; }
        int Width()  const { return width; }
        int Height() const { return height; }

    private:
        int width;
        int height;
        std::unique_ptr<val_cg::Display>     display;   // declared before device — constructed first
        std::unique_ptr<rhi::GraphicsDevice> device;    // destroyed before the window
    };

}
