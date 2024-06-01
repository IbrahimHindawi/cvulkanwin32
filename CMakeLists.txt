cmake_minimum_required(VERSION 3.10)

set(ProjectNameParam "DefaultProject" CACHE STRING "Command line argument containing the project name.")
project(${ProjectNameParam} VERSION 1.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME}
    source/main.c
    source/fileops.c
)
include_directories(source/)
add_subdirectory(extern)
target_link_libraries(${PROJECT_NAME} PUBLIC haikal)

# find_package(cglm CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE cglm::cglm)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif()

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resource ${CMAKE_CURRENT_BINARY_DIR}/resource)
