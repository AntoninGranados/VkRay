cmake_minimum_required(VERSION 3.20)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  project(VkRay LANGUAGES C CXX OBJC)
else()
  project(VkRay LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 26)
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

add_library(tinyexr STATIC "${CMAKE_SOURCE_DIR}/external/tinyexr/miniz.c")
target_include_directories(tinyexr PUBLIC
  "${CMAKE_SOURCE_DIR}/external/tinyexr"
)

# ── Engine submodule ──────────────────────────────────────────────────────────
set(VKSMOL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external")
add_subdirectory("${CMAKE_SOURCE_DIR}/external/VkSmol" engine)

# ── NFD (Native File Dialog Extended) ────────────────────────────────────────
add_subdirectory("${CMAKE_SOURCE_DIR}/external/nfd")

# ── GLM ───────────────────────────────────────────────────────────────────────
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_SOURCE_DIR}/external/glm")

# ── Sources ───────────────────────────────────────────────────────────────────
file(GLOB_RECURSE VKRAY_ALL_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
set(VKRAY_SOURCES ${VKRAY_ALL_SOURCES})
list(FILTER VKRAY_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

file(GLOB_RECURSE VKRAY_EMBEDDED_FILES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/shaders/*.glsl"
  "${CMAKE_SOURCE_DIR}/assets/fonts/*.ttf"
  "${CMAKE_SOURCE_DIR}/assets/fonts/*.otf"
  "${CMAKE_SOURCE_DIR}/assets/apertures/*.pgm"
)
list(APPEND VKRAY_EMBEDDED_FILES
  "${CMAKE_SOURCE_DIR}/src/config/parameters.json"
  "${CMAKE_SOURCE_DIR}/assets/camera/tilt_shift.glsl"
  "${CMAKE_SOURCE_DIR}/assets/scenes/default.json"
)

set(VKRAY_EMBEDDED_ARRAYS "")
set(VKRAY_EMBEDDED_ENTRIES "")
set(VKRAY_EMBEDDED_INDEX 0)
foreach(VKRAY_EMBEDDED_FILE IN LISTS VKRAY_EMBEDDED_FILES)
  file(RELATIVE_PATH VKRAY_EMBEDDED_NAME "${CMAKE_SOURCE_DIR}" "${VKRAY_EMBEDDED_FILE}")
  string(REGEX REPLACE "^(src|assets)/" "builtin:/" VKRAY_EMBEDDED_NAME "${VKRAY_EMBEDDED_NAME}")
  string(APPEND VKRAY_EMBEDDED_ARRAYS
    "constexpr unsigned char kResource${VKRAY_EMBEDDED_INDEX}[] = {\n#embed \"${VKRAY_EMBEDDED_FILE}\"\n};\n")
  string(APPEND VKRAY_EMBEDDED_ENTRIES
    "    {\"${VKRAY_EMBEDDED_NAME}\", kResource${VKRAY_EMBEDDED_INDEX}},\n")
  math(EXPR VKRAY_EMBEDDED_INDEX "${VKRAY_EMBEDDED_INDEX} + 1")
endforeach()

set(VKRAY_EMBEDDED_SOURCE "${CMAKE_BINARY_DIR}/resources.cpp")
configure_file("${CMAKE_SOURCE_DIR}/src/utils/resources.cpp.in" "${VKRAY_EMBEDDED_SOURCE}" @ONLY)
set_source_files_properties("${VKRAY_EMBEDDED_SOURCE}" PROPERTIES
  COMPILE_OPTIONS "-Wno-c23-extensions"
  OBJECT_DEPENDS "${VKRAY_EMBEDDED_FILES}"
)

add_executable(VkRay ${VKRAY_SOURCES} "${VKRAY_EMBEDDED_SOURCE}" "${CMAKE_SOURCE_DIR}/src/main.cpp")

target_include_directories(VkRay PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/external"
  "${CMAKE_BINARY_DIR}"
)

target_link_libraries(VkRay PRIVATE VkSmol nfd tinyexr glm::glm)
target_compile_options(VkRay PRIVATE -Wall -Wextra)

# stb_image_write.h/tinyexr.h are vendored single-header libs included directly as source here;
# these warnings come from their own internals, not from VkRay code.
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/core/export_service.cpp" PROPERTIES
  COMPILE_OPTIONS "-Wno-deprecated-declarations;-Wno-unused-function;-Wno-unused-parameter;-Wno-missing-field-initializers"
)
if(DEFINED ENV{VULKAN_SDK})
  set_property(TARGET VkRay PROPERTY BUILD_RPATH "$ENV{VULKAN_SDK}/lib")
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(VKRAY_BUILD_TESTS "Build the unit test suite" ON)
if(VKRAY_BUILD_TESTS)
  enable_testing()

  add_executable(VkRayTests
    "${CMAKE_SOURCE_DIR}/src/core/animation/animation_clock.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/animation/animation_store.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/animation/keyframe.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/animation/track.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/ecs/components/component_type.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/ecs/systems/physics/physics_solver.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/fields/field.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/fields/field_serializer.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/scene/asset/mesh.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/scene/scene.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/scene/scene_serializer.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/json_dsl.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/log.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/string_utils.cpp"
    "${VKRAY_EMBEDDED_SOURCE}"
    "${CMAKE_SOURCE_DIR}/tests/support/gtest_style_reporter.cpp"
    "${CMAKE_SOURCE_DIR}/tests/support/shader_plugin_stub.cpp"
    "${CMAKE_SOURCE_DIR}/tests/main.cpp"
    "${CMAKE_SOURCE_DIR}/tests/core/render/sample_accumulator_test.cpp"
    "${CMAKE_SOURCE_DIR}/tests/core/animation/animation_clock_test.cpp"
    "${CMAKE_SOURCE_DIR}/tests/core/ecs/registry_test.cpp"
    "${CMAKE_SOURCE_DIR}/tests/core/ecs/system_scheduler_test.cpp"
    "${CMAKE_SOURCE_DIR}/tests/core/scene/scene_serializer_test.cpp"
    "${CMAKE_SOURCE_DIR}/tests/utils/resources_test.cpp"
  )

  target_include_directories(VkRayTests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/external"
    "${CMAKE_BINARY_DIR}"
  )

  target_link_libraries(VkRayTests PRIVATE glm::glm Vulkan::Vulkan)

  include("${CMAKE_SOURCE_DIR}/external/doctest/cmake/doctest.cmake")
  doctest_discover_tests(VkRayTests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

  add_test(
    NAME "Golden image matches reference"
    COMMAND python3 "${CMAKE_SOURCE_DIR}/tests/golden/compare_render.py"
            "$<TARGET_FILE:VkRay>"
            "${CMAKE_SOURCE_DIR}"
  )

  add_test(
    NAME "Docs match generated output"
    COMMAND "${CMAKE_SOURCE_DIR}/tests/check_docs.sh" "$<TARGET_FILE:VkRay>"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )

  add_test(
    NAME "Source files match clang-format"
    COMMAND "${CMAKE_SOURCE_DIR}/tests/check_format.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  )
endif()
