#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Win32/Win32Misc.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
	//TODO: Implement macOS
#else
	#error No platform defined
#endif