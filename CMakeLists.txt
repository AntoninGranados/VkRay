cmake_minimum_required(VERSION 3.20)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  project(VkRay LANGUAGES C CXX OBJC)
else()
  project(VkRay LANGUAGES C CXX)
endif()

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
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/VERSION")
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

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(Vulkan REQUIRED)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/external/glfw" glfw)
set(VKRAY_GLFW_TARGET glfw)

file(GLOB VKRAY_IMGUI_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/external/imgui/*.cpp")
add_library(imgui STATIC ${VKRAY_IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
  "${CMAKE_SOURCE_DIR}/external/imgui"
)
target_link_libraries(imgui PUBLIC ${VKRAY_GLFW_TARGET} Vulkan::Vulkan)
target_compile_options(imgui PRIVATE -Wno-nontrivial-memcall)

add_library(tinyexr STATIC "${CMAKE_SOURCE_DIR}/external/tinyexr/miniz.c")
target_include_directories(tinyexr PUBLIC
  "${CMAKE_SOURCE_DIR}/external/tinyexr"
)

# ── Engine submodule ──────────────────────────────────────────────────────────
set(VKSMOL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/imgui;${CMAKE_SOURCE_DIR}/external")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/VkSmol" engine)

# ── NFD (Native File Dialog Extended) ────────────────────────────────────────
add_subdirectory("${CMAKE_SOURCE_DIR}/external/nfd")

# ── Sources ───────────────────────────────────────────────────────────────────
file(GLOB_RECURSE VKRAY_ALL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
set(VKRAY_SOURCES ${VKRAY_ALL_SOURCES})
list(FILTER VKRAY_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_executable(VkRay ${VKRAY_SOURCES} "${CMAKE_SOURCE_DIR}/src/main.cpp")

target_include_directories(VkRay PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/external"
  "${CMAKE_BINARY_DIR}"
)

target_link_libraries(VkRay PRIVATE VkSmol nfd tinyexr)
target_compile_options(VkRay PRIVATE -Wall)

set_source_files_properties("${CMAKE_SOURCE_DIR}/src/core/export_service.cpp" PROPERTIES
  COMPILE_OPTIONS "-Wno-deprecated-declarations;-Wno-unused-function"
)
target_compile_definitions(VkRay PRIVATE BUILD_DIR="${CMAKE_BINARY_DIR}")
if(DEFINED ENV{VULKAN_SDK})
  set_property(TARGET VkRay PROPERTY BUILD_RPATH "$ENV{VULKAN_SDK}/lib")
endif()

# ── Post-build ────────────────────────────────────────────────────────────────
option(VKRAY_COPY_RESOURCES "Copy res/ into the build output directory" ON)
if(VKRAY_COPY_RESOURCES)
  add_custom_command(
    TARGET VkRay POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:VkRay>/assets"
  )
endif()

add_custom_command(
  TARGET VkRay POST_BUILD
  COMMAND "${CMAKE_SOURCE_DIR}/scripts/reference_render.sh"
          "$<TARGET_FILE:VkRay>"
          "${CMAKE_SOURCE_DIR}"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Checking if a reference render is needed..."
)
