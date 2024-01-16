add_executable(RotatingCube ${CMAKE_CURRENT_SOURCE_DIR}/RotatingCube.cpp)
target_link_libraries(RotatingCube kbs)

if(KBS_CORE_DLL)
	add_compile_definitions(KBS_CORE_DLL)
endif()

if (WIN32)
	add_compile_definitions(KBS_PLATFORM_WINDOWS)
endif()

add_compile_definitions(ROTATING_CUBE_ROOT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(RotatingCube PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(TARGET RotatingCube POST_BUILD        # Adds a post-build event to deferred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        $<TARGET_FILE:kbs>      # <--this is in-file
        $<TARGET_FILE_DIR:RotatingCube>) 