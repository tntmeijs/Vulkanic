# GLFW
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(glfw)
target_link_libraries(${PROJECT_NAME} glfw)
target_include_directories(${PROJECT_NAME} PRIVATE glfw)

# GLM
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
set(GLM_INSTALL_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(glm)
target_link_libraries(${PROJECT_NAME} glm)
target_include_directories(${PROJECT_NAME} PRIVATE glm)

# Spdlog
add_subdirectory(spdlog)
target_link_libraries(${PROJECT_NAME} spdlog)
target_include_directories(${PROJECT_NAME} PRIVATE spdlog)

# Glslang
add_subdirectory(glslang)
target_link_libraries(${PROJECT_NAME} glslang)
target_link_libraries(${PROJECT_NAME} SPIRV)
target_include_directories(${PROJECT_NAME} PRIVATE glslang)

# Dear ImGui
set(IMGUI_FILES
    imgui/imgui.cpp
    imgui/imconfig.h
    imgui/imgui_draw.cpp
    imgui/imgui_internal.h
    imgui/imgui_widgets.cpp
    imgui/imgui.h
    imgui/imstb_rectpack.h
    imgui/imstb_truetype.h
    imgui/examples/imgui_impl_vulkan.cpp
    imgui/examples/imgui_impl_vulkan.h
    imgui/examples/imgui_impl_glfw.cpp
    imgui/examples/imgui_impl_glfw.h)
add_library(imgui ${IMGUI_FILES})
target_include_directories(imgui PRIVATE imgui)         # ImGui needs the ImGui root folder
target_include_directories(imgui PRIVATE glfw/include)  # ImGui needs GLFW
target_link_libraries(imgui Vulkan::Vulkan)             # ImGui needs Vulkan
target_link_libraries(${PROJECT_NAME} imgui)
target_include_directories(${PROJECT_NAME} PRIVATE imgui)

# STB (header only)
target_include_directories(${PROJECT_NAME} PRIVATE stb)

# VulkanMemoryAllocator (header only)
target_include_directories(${PROJECT_NAME} PRIVATE VulkanMemoryAllocator/src/)

# Group the generated third-party projects
if(MSVC)
    # GLFW
    set_target_properties(glfw PROPERTIES FOLDER Dependencies/glfw)
    
    # Spdlog
    set_target_properties(spdlog_headers_for_ide PROPERTIES FOLDER Dependencies/spdlog)

    # Dear ImGui
    set_target_properties(imgui PROPERTIES FOLDER Dependencies/imgui)

    # GLM
    set_target_properties(glm_static PROPERTIES FOLDER Dependencies/glm)
    set_target_properties(uninstall PROPERTIES FOLDER Dependencies/glm)
    
    # Glslang
    set_target_properties(glslang PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(glslang-default-resource-limits PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(OGLCompiler PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(OSDependent PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(SPIRV PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(SPVRemapper PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(Continuous PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(Experimental PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(Nightly PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(NightlyMemoryCheck PROPERTIES FOLDER Dependencies/glslang)
    set_target_properties(HLSL PROPERTIES FOLDER Dependencies/glslang/hlsl)
    set_target_properties(glslangValidator PROPERTIES FOLDER Dependencies/glslang/tools)
    set_target_properties(spirv-remap PROPERTIES FOLDER Dependencies/glslang/tools)
endif(MSVC)
