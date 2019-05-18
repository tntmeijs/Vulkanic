# GLFW - configure
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# GLM - configure
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
set(GLM_INSTALL_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(glm)

# Spdlog - configure
add_subdirectory(spdlog)

# Glslang - configure
add_subdirectory(glslang)

# GLFW - link
target_link_libraries(${PROJECT_NAME} glfw)
target_include_directories(${PROJECT_NAME} PRIVATE glfw)

# GLM - link
target_link_libraries(${PROJECT_NAME} glm)
target_include_directories(${PROJECT_NAME} PRIVATE glm)

# Spdlog - link
target_link_libraries(${PROJECT_NAME} spdlog)
target_include_directories(${PROJECT_NAME} PRIVATE spdlog)

# Glslang - link
target_link_libraries(${PROJECT_NAME} glslang)
target_link_libraries(${PROJECT_NAME} SPIRV)
target_include_directories(${PROJECT_NAME} PRIVATE glslang)

# Group the generated third-party projects
set_target_properties(glfw PROPERTIES FOLDER Dependencies)
set_target_properties(glm_static PROPERTIES FOLDER Dependencies)
set_target_properties(spdlog_headers_for_ide PROPERTIES FOLDER Dependencies)
