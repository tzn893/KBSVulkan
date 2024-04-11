add_subdirectory(Hello)
add_subdirectory(RotatingCube)
add_subdirectory(ComplexScene)
add_subdirectory(GIBS)
add_subdirectory(PathTracing)

set_target_properties(Hello RotatingCube ComplexScene GIBS PathTracing PROPERTIES FOLDER examples)