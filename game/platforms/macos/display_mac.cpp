//
// Created by Volkov Sergey on 02/09/2026.
//
//   ctor         NSApplication setup + menu + delegate, NSWindow, CAMetalLayer on its view
//   PumpEvents   the WndProc/PeekMessage equivalent: drain NSApp's queue without blocking,
//                translate input ourselves, forward everything else to [NSApp sendEvent:]
//   delegate     an Objective-C object built at runtime from C++ lambdas; AppKit calls it
//                for "close button pressed", "Cmd+Q", "window lost focus"
#include "display_mac.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include <AppKit/AppKit.hpp>
#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <objc/runtime.h>

#include "appkit_ext.hpp"
#include "InputDevice.h"

namespace val_cg {

namespace {

// ---- Key mapping: macOS hardware key codes (Carbon kVK_*, HIToolbox/Events.h) → Keys (VK codes) ----

struct KeyMapping { unsigned short macKey; Keys key; };

constexpr KeyMapping kKeyMap[] = {
    {0x00, Keys::A}, {0x0B, Keys::B}, {0x08, Keys::C}, {0x02, Keys::D}, {0x0E, Keys::E},
    {0x03, Keys::F}, {0x05, Keys::G}, {0x04, Keys::H}, {0x22, Keys::I}, {0x26, Keys::J},
    {0x28, Keys::K}, {0x25, Keys::L}, {0x2E, Keys::M}, {0x2D, Keys::N}, {0x1F, Keys::O},
    {0x23, Keys::P}, {0x0C, Keys::Q}, {0x0F, Keys::R}, {0x01, Keys::S}, {0x11, Keys::T},
    {0x20, Keys::U}, {0x09, Keys::V}, {0x0D, Keys::W}, {0x07, Keys::X}, {0x10, Keys::Y},
    {0x06, Keys::Z},
    {0x1D, Keys::D0}, {0x12, Keys::D1}, {0x13, Keys::D2}, {0x14, Keys::D3}, {0x15, Keys::D4},
    {0x17, Keys::D5}, {0x16, Keys::D6}, {0x1A, Keys::D7}, {0x1C, Keys::D8}, {0x19, Keys::D9},
    {0x7B, Keys::Left}, {0x7C, Keys::Right}, {0x7D, Keys::Down}, {0x7E, Keys::Up},
    {0x31, Keys::Space}, {0x24, Keys::Enter}, {0x30, Keys::Tab}, {0x33, Keys::Back},
    {0x35, Keys::Escape}, {0x75, Keys::Delete},
    {0x73, Keys::Home}, {0x77, Keys::End}, {0x74, Keys::PageUp}, {0x79, Keys::PageDown},
    {0x7A, Keys::F1}, {0x78, Keys::F2}, {0x63, Keys::F3}, {0x76, Keys::F4}, {0x60, Keys::F5},
    {0x61, Keys::F6}, {0x62, Keys::F7}, {0x64, Keys::F8}, {0x65, Keys::F9}, {0x6D, Keys::F10},
    {0x67, Keys::F11}, {0x6F, Keys::F12},
};

constexpr unsigned short kVK_Escape = 0x35;

Keys MapKey(unsigned short macKey) {
    for (const auto& m : kKeyMap)
        if (m.macKey == macKey) return m.key;
    return Keys::None;
}

// Modifier keys never send KeyDown/KeyUp, only FlagsChanged with the new modifierFlags.
// The low bits of those flags tell left from right (NX_DEVICE*KEYMASK, IOKit/hidsystem/IOLLEvent.h).
struct ModifierMapping { unsigned long deviceMask; Keys key; };

constexpr ModifierMapping kModifierMap[] = {
    {0x00000002, Keys::LeftShift},    {0x00000004, Keys::RightShift},
    {0x00000001, Keys::LeftControl},  {0x00002000, Keys::RightControl},
    {0x00000020, Keys::LeftAlt},      {0x00000040, Keys::RightAlt},       // Option
    {0x00000008, Keys::LeftWindows},  {0x00000010, Keys::RightWindows},   // Command
};

// RAWMOUSE usButtonFlags values (RI_MOUSE_*) — what InputDevice::OnMouseMove expects.
enum : int {
    kLeftDown = 0x001, kLeftUp = 0x002, kRightDown = 0x004, kRightUp = 0x008,
    kMiddleDown = 0x010, kMiddleUp = 0x020, kWheel = 0x400,
};

constexpr int kWheelDelta = 120;  // WHEEL_DELTA: one wheel notch on Windows

// ---- The delegate: an Objective-C class made at runtime ----
//
// kinds of the same as metal-cpp NSApplication
// but this is still bullshit
DisplayMac*& Owner(id self) {
    return *static_cast<DisplayMac**>(object_getIndexedIvars(self));
}

Class DelegateClass() {
    static Class cls = [] {
        Class c = objc_allocateClassPair(objc_getClass("NSObject"), "ValCgDisplayDelegate", 0);

        // NSWindowDelegate — red close button. Answer NO and let Game::Run unwind normally:
        // the window (and its CAMetalLayer) must outlive the GPU device, which Platform
        // destroys before the display.
        class_addMethod(c, sel_registerName("windowShouldClose:"),
            reinterpret_cast<IMP>(+[](id self, SEL, id) -> BOOL {
                Owner(self)->OnQuitRequested();
                return NO;
            }), "B@:@");

        // NSWindowDelegate — another app/window took keyboard focus; keys we saw go down
        // will never send their KeyUp to us.
        class_addMethod(c, sel_registerName("windowDidResignKey:"),
            reinterpret_cast<IMP>(+[](id self, SEL, id) {
                Owner(self)->OnFocusLost();
            }), "v@:@");

        // NSApplicationDelegate — Cmd+Q, Quit menu, Dock "Quit". Cancel the termination
        // (it would exit() without running destructors) and quit through the game loop.
        class_addMethod(c, sel_registerName("applicationShouldTerminate:"),
            reinterpret_cast<IMP>(+[](id self, SEL, id) -> NS::UInteger {
                Owner(self)->OnQuitRequested();
                return appkit::TerminateCancel;
            }), "Q@:@");

        objc_registerClassPair(c);
        return c;
    }();
    return cls;
}

void* CreateDelegate(DisplayMac* owner) {
    id obj = class_createInstance(DelegateClass(), sizeof(DisplayMac*));
    Owner(obj) = owner;
    return appkit::Send<void*>(obj, sel_registerName("init"));
}

// yay QMenuBar
NS::Menu* CreateMainMenu(const std::string& name) {

    NS::Menu* bar = NS::Menu::alloc()->init();
    NS::MenuItem* appItem = NS::MenuItem::alloc()->init();
    NS::Menu* appMenu = NS::Menu::alloc()->init(NS::String::string(name.c_str(), NS::UTF8StringEncoding));

    // target nil → goes up the responder chain to NSApp → applicationShouldTerminate:
    const std::string quit = "Quit " + name;
    appMenu->addItem(NS::String::string(quit.c_str(), NS::UTF8StringEncoding),
                     VALCG_SEL("terminate:"),
                     NS::String::string("q", NS::UTF8StringEncoding));

    appItem->setSubmenu(appMenu);
    bar->addItem(appItem);
    appMenu->release();
    appItem->release();
    return bar->autorelease();
}

} // namespace

// ---- DisplayMac ----

DisplayMac::DisplayMac(const std::string& name, int clientWidth, int clientHeight)
    : ClientWidth(clientWidth), ClientHeight(clientHeight)
{
    // Every metal-cpp/AppKit call that returns an autoreleased object needs a pool around it.
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    // A bare executable (no .app bundle) starts as a background process: no Dock icon,
    // no menu bar, can't take focus. ActivationPolicyRegular makes it a normal app.
    auto* app = appkit::Application::sharedApplication();
    app->setActivationPolicy(NS::ActivationPolicyRegular);

    mDelegate = CreateDelegate(this);
    app->setDelegate(mDelegate);
    app->setMainMenu(CreateMainMenu(name));

    // NSApp->run() would do this and then block forever; we own the loop, so we
    // only do the "launching" part.
    app->finishLaunching();
    // or also one of like gazillion other delegates for app launching process

    // Fixed-size window; the content rect is the client area (like AdjustWindowRect on Win32).
    mWindow = appkit::Window::create(CGRectMake(0, 0, clientWidth, clientHeight),
                                     NS::WindowStyleMaskTitled | NS::WindowStyleMaskClosable |
                                     NS::WindowStyleMaskMiniaturizable);
    mWindow->setReleasedWhenClosed(false);  // we release it ourselves in the destructor
    mWindow->setDelegate(mDelegate);
    mWindow->setAcceptsMouseMovedEvents(true);  // otherwise no MouseMoved without a button held
    mWindow->setTitle(NS::String::string(name.c_str(), NS::UTF8StringEncoding));

    // The swapchain...
    mLayer = CA::MetalLayer::layer()->retain();
    mLayer->setDrawableSize(CGSizeMake(clientWidth, clientHeight));

    // "Layer-hosting" view: setLayer BEFORE setWantsLayer, so AppKit uses our layer as-is
    // instead of creating its own.
    appkit::View* view = mWindow->contentView();
    view->setLayer(mLayer);
    view->setWantsLayer(true);

    mWindow->center();
    mWindow->makeKeyAndOrderFront(nullptr);
    app->activateIgnoringOtherApps(true);

    pool->release();
}

DisplayMac::~DisplayMac() {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    // Detach the delegate first: AppKit holds it weakly and it is about to be freed.
    appkit::Application::sharedApplication()->setDelegate(nullptr);
    mWindow->setDelegate(nullptr);
    mWindow->orderOut(nullptr);
    mWindow->close();
    mWindow->release();
    mLayer->release();
    appkit::Send<void>(mDelegate, sel_registerName("release"));

    pool->release();
}

bool DisplayMac::PumpEvents() {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    auto* app = appkit::Application::sharedApplication();
    while (appkit::Event* event = app->nextEvent()) {
        if (!HandleEvent(event))
            app->sendEvent(event);  // window dragging, close button, menus, ...
    }

    pool->release();
    return !mQuitRequested;
}

void DisplayMac::SetTitle(const char* title) {
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    mWindow->setTitle(NS::String::string(title, NS::UTF8StringEncoding));
    pool->release();
}

bool DisplayMac::CursorClientPos(int& x, int& y) const {
    const CGPoint p = mWindow->mouseLocationOutsideOfEventStream();
    WindowToClient(p.x, p.y, x, y);
    return x >= 0 && y >= 0 && x < ClientWidth && y < ClientHeight;
}

void DisplayMac::WindowToClient(double wx, double wy, int& x, int& y) const {
    // AppKit's origin is bottom-left, ours (and D3D's) top-left.
    const appkit::View* view = mWindow->contentView();
    const CGPoint p = view->convertPointFromWindow(CGPointMake(wx, wy));
    x = static_cast<int>(std::floor(p.x));
    y = static_cast<int>(std::floor(view->bounds().size.height - p.y));
}

void DisplayMac::OnFocusLost() {
    // Release everything we reported as held; their KeyUp/MouseUp go to another app.
    auto held = std::move(mHeldKeys);
    mHeldKeys.clear();
    for (Keys key : held) SendKey(key, false);
    SendMouse(kLeftUp | kRightUp | kMiddleUp, 0, 0, 0);
    mPrevModifierFlags = 0;
}

bool DisplayMac::HandleEvent(appkit::Event* event) {
    switch (event->type()) {
        //holy hell
        case appkit::EventTypeKeyDown:
            // Let Cmd+<key> reach the menu (Cmd+Q).
            if (event->modifierFlags() & NS::EventModifierFlagCommand) return false;
            HandleKey(event, true);
            return true;
        case appkit::EventTypeKeyUp:
            HandleKey(event, false);
            return true;
        case appkit::EventTypeFlagsChanged:
            HandleModifiers(event);
            return true;
        case appkit::EventTypeLeftMouseDown:
        case appkit::EventTypeLeftMouseUp:
        case appkit::EventTypeRightMouseDown:
        case appkit::EventTypeRightMouseUp:
        case appkit::EventTypeOtherMouseDown:
        case appkit::EventTypeOtherMouseUp:
        case appkit::EventTypeMouseMoved:
        case appkit::EventTypeLeftMouseDragged:
        case appkit::EventTypeRightMouseDragged:
        case appkit::EventTypeOtherMouseDragged:
        case appkit::EventTypeScrollWheel:
            HandleMouse(event);
            return false;  // still forwarded, so the title bar / close button keep working
        default:
            return false;
    }
}

void DisplayMac::HandleKey(appkit::Event* event, bool down) {
    const unsigned short macKey = event->keyCode();

    // Same as WndProc's WM_KEYDOWN: Escape quits.
    if (down && macKey == kVK_Escape) mQuitRequested = true;

    // Auto-repeat: InputDevice tracks held state itself, repeats add nothing.
    if (down && event->isARepeat()) return;

    const Keys key = MapKey(macKey);
    if (key != Keys::None) SendKey(key, down);
}

void DisplayMac::HandleModifiers(appkit::Event* event) {
    const unsigned long flags = event->modifierFlags();
    for (const auto& m : kModifierMap) {
        const bool now    = flags & m.deviceMask;
        const bool before = mPrevModifierFlags & m.deviceMask;
        if (now != before) SendKey(m.key, now);
    }
    mPrevModifierFlags = flags;
}

void DisplayMac::HandleMouse(appkit::Event* event) {
    const auto type = event->type();

    // Presses only count inside the client area (not on the title bar); releases always
    // count, so a button released outside the window doesn't stay stuck.
    int cx, cy;
    const bool inside = event->window() == mWindow && CursorClientPos(cx, cy);

    int flags = 0;
    int wheel = 0;
    switch (type) {
        case appkit::EventTypeLeftMouseDown:  if (inside) flags = kLeftDown;  break;
        case appkit::EventTypeLeftMouseUp:    flags = kLeftUp;                break;
        case appkit::EventTypeRightMouseDown: if (inside) flags = kRightDown; break;
        case appkit::EventTypeRightMouseUp:   flags = kRightUp;               break;
        case appkit::EventTypeOtherMouseDown: if (inside && event->buttonNumber() == 2) flags = kMiddleDown; break;
        case appkit::EventTypeOtherMouseUp:   if (event->buttonNumber() == 2) flags = kMiddleUp;             break;
        case appkit::EventTypeScrollWheel: {
            // Wheel reports lines, trackpad reports points (~10 per line). Scale to WHEEL_DELTA.
            const double lines = event->hasPreciseScrollingDeltas() ? event->scrollingDeltaY() / 10.0
                                                                    : event->scrollingDeltaY();
            wheel = static_cast<int>(std::lround(lines * kWheelDelta));
            flags = kWheel;
            break;
        }
        default: break;
    }

    // Relative motion, like raw input: keeps coming when the cursor is pinned at a screen
    // edge. deltaY is +down already. On a scroll event deltaX/Y are the scroll amounts,
    // so skip them there.
    int dx = 0, dy = 0;
    if (type != appkit::EventTypeScrollWheel) {
        mMouseRemX += event->deltaX();
        mMouseRemY += event->deltaY();
        dx = static_cast<int>(mMouseRemX);
        dy = static_cast<int>(mMouseRemY);
        mMouseRemX -= dx;
        mMouseRemY -= dy;
    }

    SendMouse(flags, wheel, dx, dy);
}

void DisplayMac::SendKey(Keys key, bool down) {
    if (down) {
        if (std::find(mHeldKeys.begin(), mHeldKeys.end(), key) == mHeldKeys.end())
            mHeldKeys.push_back(key);
    } else {
        std::erase(mHeldKeys, key);
    }

    if (!mInputDevice) return;
    // { MakeCode, Flags (bit 0 = key up, like RI_KEY_BREAK), VKey, Message }
    mInputDevice->OnKeyDown({0, static_cast<unsigned short>(down ? 0 : 1),
                             static_cast<unsigned short>(key), 0});
}

void DisplayMac::SendMouse(int buttonFlags, int wheelDelta, int dx, int dy) {
    if (!mInputDevice) return;
    int cx = 0, cy = 0;
    CursorClientPos(cx, cy);
    // { ButtonFlags, WheelDelta, X, Y (relative), ClientX, ClientY }
    mInputDevice->OnMouseMove({buttonFlags, wheelDelta, dx, dy, cx, cy});
}

std::unique_ptr<Display> CreateDisplay(const std::string& name, int clientWidth, int clientHeight) {
    return std::make_unique<DisplayMac>(name, clientWidth, clientHeight);
}

} // namespace val_cg
