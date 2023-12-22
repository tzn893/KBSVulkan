#pragma once

#ifdef KBS_PLATFORM_WINDOWS
    
    #ifdef KBS_CORE_DLL
        #ifdef KBS_DLL_LIBRARY
            #define KBS_API __declspec(dllexport)
        #else
            #define KBS_API __declspec(dllimport)
        #endif
    #else
        #define KBS_API
    #endif

    #define KBS_BREAK_POINT __debugbreak()
#else
    #error currntly only platform windows is supported
#endif