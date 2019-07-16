set(MISCELLANEOUS_FILES
    miscellaneous/global_settings.hpp
    miscellaneous/exceptions.hpp
    miscellaneous/vulkanic_literals.hpp
    miscellaneous/vma_impl.cpp
    miscellaneous/stb_impl.cpp)

set(CORE_FILES
    core/window.cpp
    core/window.hpp
    core/viewport.cpp
    core/viewport.hpp)

set(RENDERER_FILES
    renderer/renderer.cpp
    renderer/renderer.hpp)

set(MEMORY_MANAGER_FILES
    renderer/memory_manager/memory_manager.cpp
    renderer/memory_manager/memory_manager.hpp)

set(VULKAN_WRAPPER_FILES
    renderer/vulkan_wrapper/vulkan_utility.hpp
    renderer/vulkan_wrapper/vulkan_functions.hpp
    renderer/vulkan_wrapper/vulkan_pipeline_info.hpp
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
    renderer/vulkan_wrapper/vulkan_render_pass.hpp
    renderer/vulkan_wrapper/vulkan_command_buffer.cpp
    renderer/vulkan_wrapper/vulkan_command_buffer.hpp
    renderer/vulkan_wrapper/vulkan_command_pool.cpp
    renderer/vulkan_wrapper/vulkan_command_pool.hpp
    renderer/vulkan_wrapper/vulkan_texture.cpp
    renderer/vulkan_wrapper/vulkan_texture.hpp
    renderer/vulkan_wrapper/vulkan_texture_sampler.cpp
    renderer/vulkan_wrapper/vulkan_texture_sampler.hpp
    renderer/vulkan_wrapper/vulkan_uniform_buffer.cpp
    renderer/vulkan_wrapper/vulkan_uniform_buffer.hpp)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    ${MISCELLANEOUS_FILES}
    ${CORE_FILES}
    ${RENDERER_FILES}
    ${VULKAN_WRAPPER_FILES}
    ${MEMORY_MANAGER_FILES})

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

# Group the source files to keep the project nicely structured
source_group("main" FILES main.cpp)
source_group("miscellaneous" FILES ${MISCELLANEOUS_FILES})
source_group("core" FILES ${CORE_FILES})
source_group("renderer" FILES ${RENDERER_FILES})
source_group("vulkan_wrapper" FILES ${VULKAN_WRAPPER_FILES})
source_group("memory_manager" FILES ${MEMORY_MANAGER_FILES})
