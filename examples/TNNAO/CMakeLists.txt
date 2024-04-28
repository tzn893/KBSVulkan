add_executable(TNNAO ${CMAKE_CURRENT_SOURCE_DIR}/TNNAO.cpp 	${CMAKE_CURRENT_SOURCE_DIR}/TNNAORenderer.cpp 	${CMAKE_CURRENT_SOURCE_DIR}/TNNAORenderer.h)
target_link_libraries(TNNAO kbs)

if(KBS_CORE_DLL)
	add_compile_definitions(KBS_CORE_DLL)
endif()

if (WIN32)
	add_compile_definitions(KBS_PLATFORM_WINDOWS)
endif()

add_compile_definitions(ASSET_ROOT_DIRECTORY="${CMAKE_SOURCE_DIR}/examples/assets")
add_compile_definitions(TNNAO_ROOT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(TNNAO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(TARGET TNNAO POST_BUILD        # Adds a post-build event to deferred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        $<TARGET_FILE:kbs>      # <--this is in-file
        $<TARGET_FILE_DIR:TNNAO>) 

add_custom_command(TARGET TNNAO POST_BUILD        # Adds a post-build event to deferred
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake - E copy_if_different..."
        $<TARGET_FILE:ktx>      # <--this is in-file
        $<TARGET_FILE_DIR:TNNAO>) 