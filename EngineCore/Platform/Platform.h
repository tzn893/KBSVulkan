#pragma once

#ifdef KBS_PLATFORM_WINDOWS
    #ifdef KBS_DLL_LIBRARY
        #define KBS_API __declspec(dllexport)
    #else
        #define KBS_API __declspec(dllimport)
    #endif
#else
    #error currntly only platform windows is supported
#endif