cmake_minimum_required (VERSION 3.7)
set (CMAKE_CXX_STANDARD 17)
project (studious-octo-enigma)
find_package(Vulkan REQUIRED)
include_directories (${CMAKE_HOME_DIRECTORY}/src)
include_directories (${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Vulkan_INCLUDE_DIRS}/vulkan)
add_executable (
    main
    WIN32
    src/easylogging++.cc
    src/FileSystem.cpp
    src/main.cpp
    src/util.cpp
    src/VulkanApplication.cpp
    src/Win32.cpp
)
target_link_libraries(
    main
    ${Vulkan_LIBRARIES}
)
