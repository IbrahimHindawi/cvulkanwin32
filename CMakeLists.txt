cmake_minimum_required(VERSION 3.10)
include(CMakePrintHelpers)

set(ProjectNameParam "DefaultProject" CACHE STRING "Command line argument containing the project name.")
project(${ProjectNameParam} VERSION 1.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME} WIN32
    source/main.c
    source/fileops.c
)

find_package(Vulkan REQUIRED)

include_directories(source/)
include_directories($ENV{VULKAN_SDK}/Include)
include_directories(extern/haikal/src)
# cmake_print_properties(TARGETS ${PROJECT_NAME} PROPERTIES LOCATION INCLUDE_DIRECTORIES)

add_subdirectory(extern)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})

# find_package(cglm CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE cglm::cglm)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif()

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resource ${CMAKE_CURRENT_BINARY_DIR}/resource)
