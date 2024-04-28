add_subdirectory(Hello)
add_subdirectory(RotatingCube)
add_subdirectory(ComplexScene)
add_subdirectory(GIBS)
add_subdirectory(PathTracing)
add_subdirectory(Deferred)
add_subdirectory(TNNAO)

set_target_properties(Hello RotatingCube ComplexScene GIBS PathTracing Deferred TNNAO PROPERTIES FOLDER examples)