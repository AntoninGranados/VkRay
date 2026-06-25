cmake_minimum_required(VERSION 3.20)

project(vkRay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3")

if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
endif()

# ── Version ───────────────────────────────────────────────────────────────────
file(READ "${CMAKE_SOURCE_DIR}/VERSION" VK_RAY_VERSION_STRING)
string(STRIP "${VK_RAY_VERSION_STRING}" VK_RAY_VERSION_STRING)
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ "${VK_RAY_VERSION_STRING}")
set(VK_RAY_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(VK_RAY_VERSION_MINOR "${CMAKE_MATCH_2}")
set(VK_RAY_VERSION_PATCH "${CMAKE_MATCH_3}")
configure_file(
  "${CMAKE_SOURCE_DIR}/src/version.hpp.in"
  "${CMAKE_BINARY_DIR}/version.hpp"
)

# ── Engine submodule ──────────────────────────────────────────────────────────
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

file(GLOB VKRAY_IMGUI_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/third_party/imgui/*.cpp")
add_library(imgui_third_party STATIC ${VKRAY_IMGUI_SOURCES})
target_include_directories(imgui_third_party PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/include/imgui"
)
target_link_libraries(imgui_third_party PUBLIC ${VKRAY_GLFW_TARGET} Vulkan::Vulkan)

set(VKSMOL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include;${CMAKE_SOURCE_DIR}/include/imgui")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/engine" engine)

# ── Sources ───────────────────────────────────────────────────────────────────
file(GLOB_RECURSE VKRAY_ALL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
set(VKRAY_SOURCES ${VKRAY_ALL_SOURCES})
list(FILTER VKRAY_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
list(FILTER VKRAY_SOURCES EXCLUDE REGEX ".*/engine/.*\\.cpp$")

add_executable(vkRay ${VKRAY_SOURCES} "${CMAKE_SOURCE_DIR}/src/main.cpp")

target_include_directories(vkRay PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/include/imgui"
  "${CMAKE_BINARY_DIR}"
)

target_link_libraries(vkRay PRIVATE VkSmol)
target_compile_options(vkRay PRIVATE -Wall)
if(DEFINED ENV{VULKAN_SDK})
  set_property(TARGET vkRay PROPERTY BUILD_RPATH "$ENV{VULKAN_SDK}/lib")
endif()

# ── Post-build ────────────────────────────────────────────────────────────────
option(VKRAY_COPY_RESOURCES "Copy res/ into the build output directory" ON)
if(VKRAY_COPY_RESOURCES)
  add_custom_command(
    TARGET vkRay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/res"
            "$<TARGET_FILE_DIR:vkRay>/res"
  )
endif()

add_custom_command(
  TARGET vkRay POST_BUILD
  COMMAND "${CMAKE_SOURCE_DIR}/scripts/reference_render.sh"
          "$<TARGET_FILE:vkRay>"
          "${CMAKE_SOURCE_DIR}"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Checking if a reference render is needed..."
)
