add_library(kbs SHARED KBS.h)
add_compile_definitions(KBS_DLL_LIBRARY)

target_include_directories(kbs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
	add_compile_definitions(KBS_PLATFORM_WINDOWS)
endif()

add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Math)