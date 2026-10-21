cmake_minimum_required(VERSION 3.25)

project(VkRay LANGUAGES C CXX)
if(APPLE)
  enable_language(OBJC)
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

include("${PROJECT_SOURCE_DIR}/cmake/Version.cmake")
include("${PROJECT_SOURCE_DIR}/cmake/EmbedResources.cmake")

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(Vulkan REQUIRED)

add_library(tinyexr STATIC "${PROJECT_SOURCE_DIR}/external/tinyexr/miniz.c")
target_include_directories(tinyexr PUBLIC
  "${PROJECT_SOURCE_DIR}/external/tinyexr"
)

# ── Engine submodule ──────────────────────────────────────────────────────────
add_subdirectory("${PROJECT_SOURCE_DIR}/external/VkSmol" engine)

# ── NFD (Native File Dialog Extended) ────────────────────────────────────────
add_subdirectory("${PROJECT_SOURCE_DIR}/external/nfd")

# ── GLM ───────────────────────────────────────────────────────────────────────
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${PROJECT_SOURCE_DIR}/external/glm")

# ── Sources ───────────────────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

file(GLOB_RECURSE VKRAY_EMBEDDED_FILES CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/src/shaders/*.slang"
  "${PROJECT_SOURCE_DIR}/assets/fonts/*.ttf"
  "${PROJECT_SOURCE_DIR}/assets/fonts/*.otf"
  "${PROJECT_SOURCE_DIR}/assets/apertures/*.pgm"
)
vkray_embed_resources(VKRAY_EMBEDDED_SOURCE
  ${VKRAY_EMBEDDED_FILES}
  "${PROJECT_SOURCE_DIR}/src/config/parameters.json"
  "${PROJECT_SOURCE_DIR}/assets/camera/tilt_shift.slang"
  "${PROJECT_SOURCE_DIR}/assets/scenes/default.json"
  "${PROJECT_SOURCE_DIR}/assets/noise/blue_noise.bin"
)

set(VKRAY_LOGIC_SOURCES
  "${PROJECT_SOURCE_DIR}/src/core/animation/animation_clock.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/animation/animation_store.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/animation/keyframe.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/animation/track.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/ecs/components/component_serializer.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/ecs/components/component_type.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/ecs/systems/physics/physics_solver.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/fields/field.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/fields/field_serializer.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/fields/parameter_serializer.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/fields/parameters.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/scene/asset/mesh.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/scene/scene.cpp"
  "${PROJECT_SOURCE_DIR}/src/core/scene/scene_serializer.cpp"
  "${PROJECT_SOURCE_DIR}/src/utils/json_dsl.cpp"
  "${PROJECT_SOURCE_DIR}/src/utils/log.cpp"
  "${PROJECT_SOURCE_DIR}/src/utils/string_utils.cpp"
)

add_library(VkRayLogic STATIC ${VKRAY_LOGIC_SOURCES} "${VKRAY_EMBEDDED_SOURCE}")

target_include_directories(VkRayLogic PUBLIC
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/external"
  "${PROJECT_BINARY_DIR}"
)

target_link_libraries(VkRayLogic PUBLIC glm::glm Vulkan::Headers)
target_compile_options(VkRayLogic PRIVATE -Wall -Wextra)

file(GLOB_RECURSE VKRAY_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM VKRAY_SOURCES ${VKRAY_LOGIC_SOURCES})

add_executable(VkRay ${VKRAY_SOURCES})

target_link_libraries(VkRay PRIVATE VkRayLogic VkSmol nfd tinyexr)
target_compile_options(VkRay PRIVATE -Wall -Wextra)

# stb_image_write.h/tinyexr.h are vendored single-header libs included directly as source here;
# these warnings come from their own internals, not from VkRay code.
set_source_files_properties("${PROJECT_SOURCE_DIR}/src/core/export_service.cpp" PROPERTIES
  COMPILE_OPTIONS "-Wno-deprecated-declarations;-Wno-unused-function;-Wno-unused-parameter;-Wno-missing-field-initializers"
)
if(DEFINED ENV{VULKAN_SDK})
  set_property(TARGET VkRay PROPERTY BUILD_RPATH "$ENV{VULKAN_SDK}/lib")
endif()

add_subdirectory("${PROJECT_SOURCE_DIR}/tools")

option(VKRAY_BUILD_TESTS "Build the unit test suite" ON)
if(VKRAY_BUILD_TESTS)
  enable_testing()
  add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()
