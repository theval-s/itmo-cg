//
// Created by Val on 23.03.2026.
//

#pragma once

// DLL export decoration is an MSVC concept; on other toolchains it expands to nothing.
#if defined(_MSC_VER)
#define GAMEFRAMEWORK_API __declspec(dllexport)
#else
#define GAMEFRAMEWORK_API
#endif
