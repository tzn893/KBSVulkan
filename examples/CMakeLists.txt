add_subdirectory(Hello)
add_subdirectory(RotatingCube)
add_subdirectory(ComplexScene)

set_target_properties(Hello RotatingCube ComplexScene PROPERTIES FOLDER examples)