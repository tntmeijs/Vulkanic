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
target_include_directories(imgui INTERFACE imgui)
target_link_libraries(${PROJECT_NAME} imgui)

# STB - header only
set(STB_FILES
    stb/stb_image.h
    stb/stb_image_write.h)
add_library(stb ${STB_FILES})
set_target_properties(stb PROPERTIES LINKER_LANGUAGE CXX) # Necessary because only .h files will confuse CMAKE and make it fail when it tries to determine the linker language for the library
target_include_directories(stb INTERFACE stb)
target_link_libraries(${PROJECT_NAME} stb)

# VulkanMemoryAllocator - header only
set(VMA_FILES VulkanMemoryAllocator/src/vk_mem_alloc.h)
add_library(vma ${VMA_FILES})
set_target_properties(vma PROPERTIES LINKER_LANGUAGE CXX) # Necessary because only .h files will confuse CMAKE and make it fail when it tries to determine the linker language for the library
target_include_directories(vma INTERFACE VulkanMemoryAllocator/src)
target_link_libraries(${PROJECT_NAME} vma)

# Group the generated third-party projects
if(MSVC)
    # GLFW
    set_target_properties(glfw PROPERTIES FOLDER Dependencies/glfw)
    
    # Spdlog
    set_target_properties(spdlog_headers_for_ide PROPERTIES FOLDER Dependencies/spdlog)

    # VMA
    set_target_properties(vma PROPERTIES FOLDER Dependencies/vma)

    # Dear ImGui
    set_target_properties(imgui PROPERTIES FOLDER Dependencies/imgui)

    # STB
    set_target_properties(stb PROPERTIES FOLDER Dependencies/stb)

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
