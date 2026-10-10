file(GLOB_RECURSE VKRAY_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(VkRayTests
  ${VKRAY_TEST_SOURCES}
  "${CMAKE_SOURCE_DIR}/tools/support/shader_plugin_stub.cpp"
)

target_link_libraries(VkRayTests PRIVATE VkRayLogic)

include("${CMAKE_SOURCE_DIR}/external/doctest/cmake/doctest.cmake")
doctest_discover_tests(VkRayTests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_test(
  NAME "Golden image matches reference"
  COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/golden/compare_render.py"
          "$<TARGET_FILE:VkRay>"
          "${CMAKE_SOURCE_DIR}"
)

add_test(
  NAME "Source files match clang-format"
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/check_format.sh"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
