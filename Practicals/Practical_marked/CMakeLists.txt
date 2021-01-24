cmake_minimum_required (VERSION 3.16)
project(ISIMA_Practical_Marked)

add_executable(ISIMA_Practical_Marked include/stb_image.h src/main.cpp)
target_include_directories(ISIMA_Practical_Marked PUBLIC include)

set(RESOURCES_PATH "${CMAKE_SOURCE_DIR}/resources" CACHE FILEPATH "Path to the resource folder")
file(TO_CMAKE_PATH "${RESOURCES_PATH}" RESOURCES_PATH_NORMALIZED)
add_definitions(-DRESOURCES_PATH="${RESOURCES_PATH_NORMALIZED}")

set_property(TARGET ISIMA_Practical_Marked PROPERTY CXX_STANDARD 17)

add_subdirectory(third_party/glfw EXCLUDE_FROM_ALL)
target_link_libraries(ISIMA_Practical_Marked PRIVATE glfw)

add_definitions(-DGLEW_STATIC)
add_subdirectory(third_party/glew-cmake EXCLUDE_FROM_ALL)
target_link_libraries(ISIMA_Practical_Marked PRIVATE libglew_static)

add_subdirectory(third_party/glm EXCLUDE_FROM_ALL)
target_link_libraries(ISIMA_Practical_Marked PRIVATE glm)