if(KBS_CORE_DLL)
	add_library(kbs SHARED KBS.h)

	add_compile_definitions(KBS_CORE_DLL)
	add_compile_definitions(KBS_DLL_LIBRARY)
else()
	add_library(kbs KBS.h)
endif()

target_link_libraries(kbs tinygltf ktx)

add_compile_definitions(KBS_ROOT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(kbs vkrg spdlog)
target_include_directories(kbs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/vkrg/src)

target_sources(kbs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Common.h)

if (WIN32)
	add_compile_definitions(KBS_PLATFORM_WINDOWS)
endif()

add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Math)
add_subdirectory(Scene)
add_subdirectory(Renderer)
add_subdirectory(Asset)