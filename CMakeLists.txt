cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)

option(KBS_ENABLE_TEST "enable building kbs tests" On)
option(KBS_ENABLE_EXAMPLE "enable building kbs examples" Off)
option(KBS_CORE_DLL "compile kbs core to dll" Off)

project(kbs)

add_subdirectory(spdlog)
add_subdirectory(vkrg)
set_target_properties(gvk vkrg PROPERTIES FOLDER vk)
add_subdirectory(EngineCore)

message("----------------------------examples-----------------------")

if(KBS_ENABLE_EXAMPLE)
	add_subdirectory(examples)
endif()

if(KBS_ENABLE_TEST)
	add_subdirectory(tests)
endif()
		
