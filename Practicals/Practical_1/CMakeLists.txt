cmake_minimum_required (VERSION 3.16)
project(ISIMA_Practical_1)

add_executable(ISIMA_Practical_1 src/main.cpp)

set_property(TARGET ISIMA_Practical_1 PROPERTY CXX_STANDARD 11)

add_subdirectory(third_party/glfw EXCLUDE_FROM_ALL)
target_link_libraries(ISIMA_Practical_1 PRIVATE glfw)

add_definitions(-DGLEW_STATIC)
add_subdirectory(third_party/glew-cmake EXCLUDE_FROM_ALL)
target_link_libraries(ISIMA_Practical_1 PRIVATE libglew_static)

add_subdirectory(third_party/glm EXCLUDE_FROM_ALL)
target_link_libraries(ISIMA_Practical_1 PRIVATE glm)