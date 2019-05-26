set(MISCELLANEOUS_FILES
    miscellaneous/global_settings.hpp
    miscellaneous/exceptions.hpp
    miscellaneous/stb_defines.hpp)

set(CORE_FILES
    core/window.cpp
    core/window.hpp
    core/viewport.cpp
    core/viewport.hpp)

set(RENDERER_FILES
    renderer/renderer.cpp
    renderer/renderer.hpp)

set(VULKAN_WRAPPER_FILES
    renderer/vulkan_wrapper/vulkan_utility.hpp
    renderer/vulkan_wrapper/vulkan_enums.hpp
    renderer/vulkan_wrapper/vulkan_structures.hpp
    renderer/vulkan_wrapper/vulkan_functions.hpp
    renderer/vulkan_wrapper/vulkan_instance.cpp
    renderer/vulkan_wrapper/vulkan_instance.hpp
    renderer/vulkan_wrapper/vulkan_debug_messenger.cpp
    renderer/vulkan_wrapper/vulkan_debug_messenger.hpp
    renderer/vulkan_wrapper/vulkan_device.cpp
    renderer/vulkan_wrapper/vulkan_device.hpp
    renderer/vulkan_wrapper/vulkan_swapchain.cpp
    renderer/vulkan_wrapper/vulkan_swapchain.hpp
    renderer/vulkan_wrapper/vulkan_shader.cpp
    renderer/vulkan_wrapper/vulkan_shader.hpp
    renderer/vulkan_wrapper/vulkan_pipeline.cpp
    renderer/vulkan_wrapper/vulkan_pipeline.hpp
    renderer/vulkan_wrapper/vulkan_render_pass.cpp
    renderer/vulkan_wrapper/vulkan_render_pass.hpp)

# Find Vulkan on the system
find_package(Vulkan REQUIRED)

# Use the Vulkan Windows extension
if(MSVC)
	add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
endif(MSVC)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    ${MISCELLANEOUS_FILES}
    ${CORE_FILES}
    ${RENDERER_FILES}
    ${VULKAN_WRAPPER_FILES})

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

# Group the source files to keep the project nicely structured
source_group("main" FILES main.cpp)
source_group("miscellaneous" FILES ${MISCELLANEOUS_FILES})
source_group("core" FILES ${CORE_FILES})
source_group("renderer" FILES ${RENDERER_FILES})
source_group("vulkan_wrapper" FILES ${VULKAN_WRAPPER_FILES})
