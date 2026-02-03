cmake_minimum_required(VERSION 3.20)

project(vkRay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3")

if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
endif()

file(GLOB_RECURSE VKRAY_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(vkRay ${VKRAY_SOURCES})

target_include_directories(vkRay PRIVATE
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/include/imgui"
)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

if(TARGET glfw)
  set(VKRAY_GLFW_TARGET glfw)
elseif(TARGET glfw3)
  set(VKRAY_GLFW_TARGET glfw3)
else()
  find_library(VKRAY_GLFW_LIBRARY glfw)
  set(VKRAY_GLFW_TARGET ${VKRAY_GLFW_LIBRARY})
endif()

target_link_libraries(vkRay PRIVATE ${VKRAY_GLFW_TARGET} Vulkan::Vulkan)
target_compile_options(vkRay PRIVATE -Wall)

if(APPLE)
  target_link_libraries(vkRay PRIVATE
    "-framework Accelerate"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework Metal"
    "-framework QuartzCore"
  )
endif()

if(DEFINED ENV{VULKAN_SDK})
  set_property(TARGET vkRay PROPERTY BUILD_RPATH "$ENV{VULKAN_SDK}/lib")
endif()

option(VKRAY_COPY_RESOURCES "Copy res/ into the build output directory" ON)
if(VKRAY_COPY_RESOURCES)
  add_custom_command(
    TARGET vkRay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/res"
            "$<TARGET_FILE_DIR:vkRay>/res"
  )
endif()
