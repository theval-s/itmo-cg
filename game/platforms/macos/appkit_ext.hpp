//
// AppKit pieces missing from metal-cpp-extensions.
//
// external/metal-cpp-extensions/AppKit is not full implementation of AppKit
// but we also need events and input
//
// NOTE: god I should've just used SDL2 or smth to create a window....
//
// Just does it the same way other headers do it (by using sendMessage and invoking objc methods lol)
//
// The Apple docs are the reference for everything here: search the method name on
// developer.apple.com (e.g. "NSEvent deltaX").
//
#pragma once

#include <AppKit/AppKit.hpp>
#include <Foundation/Foundation.hpp>
#include <CoreGraphics/CGGeometry.h>
#include <objc/runtime.h>

// sel_registerName once per call site.
#define VALCG_SEL(name) ([] { static SEL s = sel_registerName(name); return s; }())

namespace val_cg::appkit {

    // Untyped objc_msgSend, for the odd call that has no wrapper (class methods, runtime-made
    // objects). NS::Object::sendMessage is protected; this only re-exports it.
    struct Messenger : NS::Object {
        using NS::Object::sendMessage;
    };

    template <typename Ret, typename... Args>
    Ret Send(const void* obj, SEL selector, Args... args) {
        return Messenger::sendMessage<Ret>(obj, selector, args...);
    }

    // NSEventType: only the values we handle.
    _NS_ENUM(NS::UInteger, EventType) {
        EventTypeLeftMouseDown     = 1,
        EventTypeLeftMouseUp       = 2,
        EventTypeRightMouseDown    = 3,
        EventTypeRightMouseUp      = 4,
        EventTypeMouseMoved        = 5,
        EventTypeLeftMouseDragged  = 6,
        EventTypeRightMouseDragged = 7,
        EventTypeKeyDown           = 10,
        EventTypeKeyUp             = 11,
        EventTypeFlagsChanged      = 12,
        EventTypeScrollWheel       = 22,
        EventTypeOtherMouseDown    = 25,
        EventTypeOtherMouseUp      = 26,
        EventTypeOtherMouseDragged = 27,
    };

    constexpr NS::UInteger EventMaskAny = NS::UIntegerMax;

    // NSApplicationTerminateReply
    _NS_ENUM(NS::UInteger, TerminateReply) {
        TerminateCancel = 0,
        TerminateNow    = 1,
        TerminateLater  = 2,
    };

    class Window;

    class Event : public NS::Referencing<Event> {
    public:
        EventType     type() const;
        NS::UInteger  modifierFlags() const;      // NS::EventModifierFlag* (NSMenuItem.hpp) + device-dependent low bits
        unsigned short keyCode() const;           // hardware key code (Carbon kVK_*), layout-independent
        bool          isARepeat() const;
        NS::Integer   buttonNumber() const;       // 0 left, 1 right, 2 middle, ...
        CGFloat       deltaX() const;             // relative motion, +x right
        CGFloat       deltaY() const;             // relative motion, +y DOWN (unlike locationInWindow)
        CGFloat       scrollingDeltaY() const;
        bool          hasPreciseScrollingDeltas() const;  // trackpad (points) vs wheel (lines)
        CGPoint       locationInWindow() const;   // window coords, origin bottom-left
        Window*       window() const;
    };

    class View : public NS::Referencing<View, NS::View> {
    public:
        void    setLayer(const void* pLayer);     // a CALayer*, e.g. CA::MetalLayer*
        void    setWantsLayer(bool wantsLayer);
        CGRect  bounds() const;
        // convertPoint:fromView:nil — window coords → this view's coords.
        CGPoint convertPointFromWindow(CGPoint point) const;
    };

    class Window : public NS::Referencing<Window, NS::Window> {
    public:
        static Window* create(CGRect contentRect, NS::WindowStyleMask styleMask);

        void    setDelegate(const void* pDelegate);   // a runtime-made object, see display_mac.cpp
        void    setReleasedWhenClosed(bool released);
        void    setAcceptsMouseMovedEvents(bool accepts);
        void    center();
        void    orderOut(const NS::Object* pSender);
        View*   contentView() const;
        CGPoint mouseLocationOutsideOfEventStream() const;  // window coords, origin bottom-left
    };

    class Application : public NS::Referencing<Application, NS::Application> {
    public:
        static Application* sharedApplication();

        void   setDelegate(const void* pDelegate);    // a runtime-made object, see display_mac.cpp
        void   finishLaunching();
        // Non-blocking: the next queued event, or nullptr. Autoreleased — call inside a pool.
        Event* nextEvent();
        void   sendEvent(const Event* pEvent);
    };

}

// ---- Event ----

_NS_INLINE val_cg::appkit::EventType val_cg::appkit::Event::type() const {
    return Object::sendMessage<EventType>(this, VALCG_SEL("type"));
}

_NS_INLINE NS::UInteger val_cg::appkit::Event::modifierFlags() const {
    return Object::sendMessage<NS::UInteger>(this, VALCG_SEL("modifierFlags"));
}

_NS_INLINE unsigned short val_cg::appkit::Event::keyCode() const {
    return Object::sendMessage<unsigned short>(this, VALCG_SEL("keyCode"));
}

_NS_INLINE bool val_cg::appkit::Event::isARepeat() const {
    return Object::sendMessage<bool>(this, VALCG_SEL("isARepeat"));
}

_NS_INLINE NS::Integer val_cg::appkit::Event::buttonNumber() const {
    return Object::sendMessage<NS::Integer>(this, VALCG_SEL("buttonNumber"));
}

_NS_INLINE CGFloat val_cg::appkit::Event::deltaX() const {
    return Object::sendMessage<CGFloat>(this, VALCG_SEL("deltaX"));
}

_NS_INLINE CGFloat val_cg::appkit::Event::deltaY() const {
    return Object::sendMessage<CGFloat>(this, VALCG_SEL("deltaY"));
}

_NS_INLINE CGFloat val_cg::appkit::Event::scrollingDeltaY() const {
    return Object::sendMessage<CGFloat>(this, VALCG_SEL("scrollingDeltaY"));
}

_NS_INLINE bool val_cg::appkit::Event::hasPreciseScrollingDeltas() const {
    return Object::sendMessage<bool>(this, VALCG_SEL("hasPreciseScrollingDeltas"));
}

_NS_INLINE CGPoint val_cg::appkit::Event::locationInWindow() const {
    return Object::sendMessage<CGPoint>(this, VALCG_SEL("locationInWindow"));
}

_NS_INLINE val_cg::appkit::Window* val_cg::appkit::Event::window() const {
    return Object::sendMessage<Window*>(this, VALCG_SEL("window"));
}

// ---- View ----

_NS_INLINE void val_cg::appkit::View::setLayer(const void* pLayer) {
    Object::sendMessage<void>(this, VALCG_SEL("setLayer:"), pLayer);
}

_NS_INLINE void val_cg::appkit::View::setWantsLayer(bool wantsLayer) {
    Object::sendMessage<void>(this, VALCG_SEL("setWantsLayer:"), wantsLayer);
}

_NS_INLINE CGRect val_cg::appkit::View::bounds() const {
    return Object::sendMessage<CGRect>(this, VALCG_SEL("bounds"));
}

_NS_INLINE CGPoint val_cg::appkit::View::convertPointFromWindow(CGPoint point) const {
    return Object::sendMessage<CGPoint>(this, VALCG_SEL("convertPoint:fromView:"), point, static_cast<const void*>(nullptr));
}

// ---- Window ----

_NS_INLINE val_cg::appkit::Window* val_cg::appkit::Window::create(CGRect contentRect, NS::WindowStyleMask styleMask) {
    NS::Window* window = NS::Window::alloc()->init(contentRect, styleMask, NS::BackingStoreBuffered, false);
    return static_cast<Window*>(window);
}

_NS_INLINE void val_cg::appkit::Window::setDelegate(const void* pDelegate) {
    Object::sendMessage<void>(this, VALCG_SEL("setDelegate:"), pDelegate);
}

_NS_INLINE void val_cg::appkit::Window::setReleasedWhenClosed(bool released) {
    Object::sendMessage<void>(this, VALCG_SEL("setReleasedWhenClosed:"), released);
}

_NS_INLINE void val_cg::appkit::Window::setAcceptsMouseMovedEvents(bool accepts) {
    Object::sendMessage<void>(this, VALCG_SEL("setAcceptsMouseMovedEvents:"), accepts);
}

_NS_INLINE void val_cg::appkit::Window::center() {
    Object::sendMessage<void>(this, VALCG_SEL("center"));
}

_NS_INLINE void val_cg::appkit::Window::orderOut(const NS::Object* pSender) {
    Object::sendMessage<void>(this, VALCG_SEL("orderOut:"), pSender);
}

_NS_INLINE val_cg::appkit::View* val_cg::appkit::Window::contentView() const {
    return Object::sendMessage<View*>(this, VALCG_SEL("contentView"));
}

_NS_INLINE CGPoint val_cg::appkit::Window::mouseLocationOutsideOfEventStream() const {
    return Object::sendMessage<CGPoint>(this, VALCG_SEL("mouseLocationOutsideOfEventStream"));
}

// ---- Application ----

_NS_INLINE val_cg::appkit::Application* val_cg::appkit::Application::sharedApplication() {
    return static_cast<Application*>(NS::Application::sharedApplication());
}

_NS_INLINE void val_cg::appkit::Application::setDelegate(const void* pDelegate) {
    Object::sendMessage<void>(this, VALCG_SEL("setDelegate:"), pDelegate);
}

_NS_INLINE void val_cg::appkit::Application::finishLaunching() {
    Object::sendMessage<void>(this, VALCG_SEL("finishLaunching"));
}

_NS_INLINE val_cg::appkit::Event* val_cg::appkit::Application::nextEvent() {
    // [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast]
    //                       inMode:NSDefaultRunLoopMode dequeue:YES]
    // distantPast = don't wait. kCFRunLoopDefaultMode is the same string as NSDefaultRunLoopMode.
    static NS::Date* distantPast = Object::sendMessage<NS::Date*>(objc_lookUpClass("NSDate"), VALCG_SEL("distantPast"))->retain();
    return Object::sendMessage<Event*>(this, VALCG_SEL("nextEventMatchingMask:untilDate:inMode:dequeue:"),
                                       EventMaskAny, distantPast,
                                       static_cast<const void*>(kCFRunLoopDefaultMode), true);
}

_NS_INLINE void val_cg::appkit::Application::sendEvent(const Event* pEvent) {
    Object::sendMessage<void>(this, VALCG_SEL("sendEvent:"), pEvent);
}
