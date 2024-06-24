cmake_minimum_required(VERSION 3.28)
project(ref_vk DESCRIPTION "Xash3D Vulkan renderer.")

set(CMAKE_CXX_STANDARD 17)

# Include headers
include_directories(include)

function(FIND_LIB_FUNC)
    find_package(vulkan REQUIRED) # Vulkan
    find_package(glm CONFIG REQUIRED) # GLM
    find_package(SDL2 CONFIG REQUIRED) # SDL2
endfunction(FIND_LIB_FUNC)

function(ADD_LIB_FUNC name)
    target_link_libraries(${name}
            Vulkan::Vulkan
            glm::glm-header-only
            $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
            $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    )
    message(${name})
endfunction(ADD_LIB_FUNC)


FIND_LIB_FUNC()
add_library(${PROJECT_NAME} SHARED
        src/ref_vk.cpp
        include/common/typedef.h
        include/common/VulkanDebug.h
        src/common/VulkanDebug.cpp
        include/common/vktools.h
)
ADD_LIB_FUNC(${PROJECT_NAME})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Test
include_directories(include)
add_executable(test01 test/test01.cpp)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
target_link_libraries(test01 ${PROJECT_NAME})

enable_testing()
add_test(NAME test01
        COMMAND $<TARGET_FILE:test01>
)
