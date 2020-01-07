cmake_minimum_required (VERSION 3.7)
project (studious-octo-enigma)
find_package(Vulkan REQUIRED)
include_directories (${CMAKE_HOME_DIRECTORY}/src)
include_directories (${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Vulkan_INCLUDE_DIRS}/vulkan)
add_executable (
    main
    lib/easylogging++.cc
    src/main.cpp
    src/VulkanApplication.cpp
)
target_link_libraries(
    main
    ${Vulkan_LIBRARIES}
)
