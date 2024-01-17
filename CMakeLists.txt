cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)

option(KBS_ENABLE_TEST "enable building kbs tests" On)
option(KBS_ENABLE_EXAMPLE "enable building kbs examples" Off)
option(KBS_CORE_DLL "compile kbs core to dll" Off)

project(kbs)

set(KTX_FEATURE_TESTS Off CACHE BOOL "Turn off KTX_FEATURE_TESTS")
add_subdirectory(KTX-Software)
add_subdirectory(tinygltf)
add_subdirectory(spdlog)
add_subdirectory(vkrg)
set_target_properties(gvk vkrg PROPERTIES FOLDER vk)

set_target_properties(ktx tinygltf ktx_read ktx_version ktx2check ktx2check_version ktx2ktx2 ktx2ktx2_version ktxinfo ktxinfo_version ktxsc ktxsc_version ktxtools ktxtools_version imageio loader_example makedfd2vk makevk2dfd mkvk mkvkformatfiles obj_basisu_cbind objUtil toktx toktx_version fmt PROPERTIES FOLDER utils)

add_subdirectory(EngineCore)

message("----------------------------examples-----------------------")

if(KBS_ENABLE_EXAMPLE)
	add_subdirectory(examples)
endif()

if(KBS_ENABLE_TEST)
	add_subdirectory(tests)
endif()
		
