#pragma once

#include "math.hpp"
#include <unordered_set>
#include "Keys.h"
#include "Delegates.h"
#include "../exports.h"

namespace val_cg {
	class Game;
	class Display32;


	class GAMEFRAMEWORK_API InputDevice
	{
		friend class Game;
		friend class Display32;

		Game* game;

		std::unordered_set<Keys>* keys;

	public:

		struct MouseMoveEventArgs
		{
			math::Vector2 Position;
			math::Vector2 Offset;
			int WheelDelta;
		};

		math::Vector2 MousePosition;
		math::Vector2 MouseOffset;
		int MouseWheelDelta;

		MulticastDelegate<const MouseMoveEventArgs&> MouseMove;

	public:

		InputDevice(Game* inGame);
		~InputDevice();


		void AddPressedKey(Keys key);
		void RemovePressedKey(Keys key);
		bool IsKeyDown(Keys key);

	protected:
		// Event structs below are the display → InputDevice contract. They carry
		// plain ints so this header stays free of <windows.h>; the Win32 display
		// fills them from RAWINPUT, a Mac display will fill them from NSEvent.
		struct KeyboardInputEventArgs {
			/*
			 * The "make" scan code (key depression).
			 */
			unsigned short MakeCode;

			/*
			 * The flags field indicates a "break" (key release) and other
			 * miscellaneous scan code information defined in ntddkbd.h.
			 */
			unsigned short Flags;

			unsigned short VKey;      // virtual-key code — maps 1:1 onto the Keys enum
			unsigned int   Message;
		};

		enum class MouseButtonFlags
		{
			/// <unmanaged>RI_MOUSE_LEFT_BUTTON_DOWN</unmanaged>
			LeftButtonDown = 1,
			/// <unmanaged>RI_MOUSE_LEFT_BUTTON_UP</unmanaged>
			LeftButtonUp = 2,
			/// <unmanaged>RI_MOUSE_RIGHT_BUTTON_DOWN</unmanaged>
			RightButtonDown = 4,
			/// <unmanaged>RI_MOUSE_RIGHT_BUTTON_UP</unmanaged>
			RightButtonUp = 8,
			/// <unmanaged>RI_MOUSE_MIDDLE_BUTTON_DOWN</unmanaged>
			MiddleButtonDown = 16, // 0x00000010
			/// <unmanaged>RI_MOUSE_MIDDLE_BUTTON_UP</unmanaged>
			MiddleButtonUp = 32, // 0x00000020
			/// <unmanaged>RI_MOUSE_BUTTON_1_DOWN</unmanaged>
			Button1Down = LeftButtonDown, // 0x00000001
			/// <unmanaged>RI_MOUSE_BUTTON_1_UP</unmanaged>
			Button1Up = LeftButtonUp, // 0x00000002
			/// <unmanaged>RI_MOUSE_BUTTON_2_DOWN</unmanaged>
			Button2Down = RightButtonDown, // 0x00000004
			/// <unmanaged>RI_MOUSE_BUTTON_2_UP</unmanaged>
			Button2Up = RightButtonUp, // 0x00000008
			/// <unmanaged>RI_MOUSE_BUTTON_3_DOWN</unmanaged>
			Button3Down = MiddleButtonDown, // 0x00000010
			/// <unmanaged>RI_MOUSE_BUTTON_3_UP</unmanaged>
			Button3Up = MiddleButtonUp, // 0x00000020
			/// <unmanaged>RI_MOUSE_BUTTON_4_DOWN</unmanaged>
			Button4Down = 64, // 0x00000040
			/// <unmanaged>RI_MOUSE_BUTTON_4_UP</unmanaged>
			Button4Up = 128, // 0x00000080
			/// <unmanaged>RI_MOUSE_BUTTON_5_DOWN</unmanaged>
			Button5Down = 256, // 0x00000100
			/// <unmanaged>RI_MOUSE_BUTTON_5_UP</unmanaged>
			Button5Up = 512, // 0x00000200
			/// <unmanaged>RI_MOUSE_WHEEL</unmanaged>
			MouseWheel = 1024, // 0x00000400
			/// <unmanaged>RI_MOUSE_HWHEEL</unmanaged>
			Hwheel = 2048, // 0x00000800

			None = 0,
		};
		struct RawMouseEventArgs
		{
			int ButtonFlags;   // MouseButtonFlags bitmask (RI_MOUSE_* values)
			int WheelDelta;
			int X;             // relative motion since the previous event
			int Y;
			int ClientX;       // cursor position in client pixels, origin top-left
			int ClientY;
		};

	public:
		void OnKeyDown(KeyboardInputEventArgs args);
		void OnMouseMove(RawMouseEventArgs args);
	};

}