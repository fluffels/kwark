cmake_minimum_required (VERSION 3.7)
set (CMAKE_CXX_STANDARD 17)

project (studious-octo-enigma)

find_package(Vulkan REQUIRED)

set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
file(GLOB_RECURSE GLSL_FILES "shaders/*.vert" "shaders/*.frag")
foreach(GLSL_FILE ${GLSL_FILES})
    set(SPIRV_FILE "${GLSL_FILE}.spv")
    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL_FILE} -o ${SPIRV_FILE}
        DEPENDS ${GLSL_FILE}
    )
    list(APPEND SPIRV_FILES ${SPIRV_FILE})
endforeach(GLSL_FILE)
add_custom_target(Shaders DEPENDS ${SPIRV_FILES})

include_directories (${CMAKE_HOME_DIRECTORY}/src)
include_directories (${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Vulkan_INCLUDE_DIRS}/vulkan)
add_executable (
    main
    WIN32
    Shaders
    src/easylogging++.cc
    src/main.cpp
    src/FileSystem.cpp
    src/util.cpp
    src/VulkanApplication.cpp
    src/Win32.cpp
)
target_link_libraries(
    main
    ${Vulkan_LIBRARIES}
)
