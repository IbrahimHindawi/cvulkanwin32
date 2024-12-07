cmake_minimum_required(VERSION 3.10)
include(CMakePrintHelpers)

project(cvulkanwin32 VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#---------------------------------------------------------------------------------------------------
# C
#---------------------------------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    source/main.c
    source/fileops.c
)

find_package(Vulkan REQUIRED)

include_directories(source/)
include_directories($ENV{VULKAN_SDK}/Include)
include_directories(extern/haikal/src)
include_directories(extern/haikal/src/meta/gen)
include_directories(extern/glfw/include)
# cmake_print_properties(TARGETS ${PROJECT_NAME} PROPERTIES LOCATION INCLUDE_DIRECTORIES)

add_subdirectory(extern)
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})

# find_package(cglm CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE cglm::cglm)

# find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets)
