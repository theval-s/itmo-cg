#include "InputDevice.h"
#include <iostream>


using namespace val_cg::math;

namespace val_cg {
	InputDevice::InputDevice(Game* inGame) : game(inGame)
	{
		keys = new std::unordered_set<Keys>();
		// Raw-input registration lives in the display (DisplayWin32::RegisterRawInput);
		// the display delivers translated events here via OnKeyDown / OnMouseMove.
	}

	InputDevice::~InputDevice()
	{
		delete keys;
	}

	void InputDevice::OnKeyDown(KeyboardInputEventArgs args)
	{
		bool Break = args.Flags & 0x01;

		auto key = static_cast<Keys>(args.VKey);

		if (args.MakeCode == 42) key = Keys::LeftShift;
		if (args.MakeCode == 54) key = Keys::RightShift;

		if(Break) {
			if(keys->count(key))	RemovePressedKey(key);
		} else {
			if (!keys->count(key))	AddPressedKey(key);
		}
	}

	void InputDevice::OnMouseMove(RawMouseEventArgs args)
	{
		if(args.ButtonFlags & static_cast<int>(MouseButtonFlags::LeftButtonDown))
			AddPressedKey(Keys::LeftButton);
		if (args.ButtonFlags & static_cast<int>(MouseButtonFlags::LeftButtonUp))
			RemovePressedKey(Keys::LeftButton);
		if (args.ButtonFlags & static_cast<int>(MouseButtonFlags::RightButtonDown))
			AddPressedKey(Keys::RightButton);
		if (args.ButtonFlags & static_cast<int>(MouseButtonFlags::RightButtonUp))
			RemovePressedKey(Keys::RightButton);
		if (args.ButtonFlags & static_cast<int>(MouseButtonFlags::MiddleButtonDown))
			AddPressedKey(Keys::MiddleButton);
		if (args.ButtonFlags & static_cast<int>(MouseButtonFlags::MiddleButtonUp))
			RemovePressedKey(Keys::MiddleButton);

		MouseOffset     = Vector2(static_cast<float>(args.X), static_cast<float>(args.Y));
		MousePosition   = Vector2(static_cast<float>(args.ClientX), static_cast<float>(args.ClientY));
		MouseWheelDelta = args.WheelDelta;

		const MouseMoveEventArgs moveArgs = {MousePosition, MouseOffset, MouseWheelDelta};

		MouseMove.Broadcast(moveArgs);
	}

	void InputDevice::AddPressedKey(Keys key)
	{
		keys->insert(key);
	}

	void InputDevice::RemovePressedKey(Keys key)
	{
		keys->erase(key);
	}

	bool InputDevice::IsKeyDown(Keys key)
	{
		return keys->count(key);
	}
}
