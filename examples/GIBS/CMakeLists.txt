add_executable(GIBS ${CMAKE_CURRENT_SOURCE_DIR}/GIBS.cpp)
target_link_libraries(GIBS kbs)

if(KBS_CORE_DLL)
	add_compile_definitions(KBS_CORE_DLL)
endif()

if (WIN32)
	add_compile_definitions(KBS_PLATFORM_WINDOWS)
endif()

add_compile_definitions(ASSET_ROOT_DIRECTORY="${CMAKE_SOURCE_DIR}/examples/assets")
add_compile_definitions(GIBS_ROOT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(GIBS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(TARGET GIBS POST_BUILD        # Adds a post-build event to deferred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        $<TARGET_FILE:kbs>      # <--this is in-file
        $<TARGET_FILE_DIR:GIBS>) 

add_custom_command(TARGET GIBS POST_BUILD        # Adds a post-build event to deferred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        $<TARGET_FILE:ktx>      # <--this is in-file
        $<TARGET_FILE_DIR:GIBS>) 