cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)

option(KBS_ENABLE_EXAMPLE "enable building vulkan render graph examples" Off)

project(kbs)

add_subdirectory(EngineCore)

message("----------------------------examples-----------------------")

if(KBS_ENABLE_EXAMPLE)
	add_subdirectory(examples)
endif()
