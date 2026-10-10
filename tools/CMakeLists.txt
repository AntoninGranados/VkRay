add_executable(VkRayDocs
  "${CMAKE_CURRENT_SOURCE_DIR}/docs/main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/support/shader_plugin_stub.cpp"
)

target_link_libraries(VkRayDocs PRIVATE VkRayLogic)

add_custom_command(TARGET VkRayDocs POST_BUILD
  COMMAND VkRayDocs "${CMAKE_SOURCE_DIR}/docs"
  COMMENT "Generating docs/parameters.md and docs/components.md"
)
