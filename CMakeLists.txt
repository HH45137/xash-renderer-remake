cmake_minimum_required(VERSION 3.28)
project(ref_vk DESCRIPTION "Xash3D Vulkan renderer.")

set(CMAKE_CXX_STANDARD 17)

# Include headers
include_directories(include)

# Libs
add_library(${PROJECT_NAME} SHARED
        src/library.cpp
)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Test
include_directories(include)
add_executable(test01 test/test01.cpp)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

enable_testing()
add_test(NAME test01
        COMMAND $<TARGET_FILE:test01>
)
