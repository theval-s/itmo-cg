//
// Minimal Win32 PODs so DirectXTK's SimpleMath compiles on non-Windows.
//
#pragma once
#ifndef _WIN32
typedef long         LONG;
typedef unsigned int UINT;
struct RECT { LONG left, top, right, bottom; };
#endif
