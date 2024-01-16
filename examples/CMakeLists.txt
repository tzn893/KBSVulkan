add_subdirectory(Hello)
add_subdirectory(RotatingCube)

set_target_properties(Hello RotatingCube PROPERTIES FOLDER examples)