add_executable(Hello ${CMAKE_CURRENT_SOURCE_DIR}/Hello.cpp)
target_link_libraries(Hello kbs)

if (WIN32)
	add_compile_definitions(KBS_PLATFORM_WINDOWS)
endif()

target_include_directories(Hello PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(TARGET Hello POST_BUILD        # Adds a post-build event to deferred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        $<TARGET_FILE:kbs>      # <--this is in-file
        $<TARGET_FILE_DIR:Hello>) 