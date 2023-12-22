add_subdirectory(glm)
target_link_libraries(kbs glm)

file(GLOB KBS_MATH_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB KBS_MATH_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set_source_files_properties(${KBS_MATH_SOURCE} ${KBS_MATH_HEADER} PROPERTIES FOLDER Math)

target_sources(kbs PRIVATE ${KBS_MATH_SOURCE} ${KBS_MATH_HEADER})