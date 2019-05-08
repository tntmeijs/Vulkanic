set(MISCELLANEOUS_FILES
    miscellaneous/global_settings.hpp)

set(RENDERER_FILES
    renderer/renderer.cpp
    renderer/renderer.hpp)

set(VULKAN_WRAPPER_FILES
    renderer/vulkan_wrapper/vk_utility.hpp
    renderer/vulkan_wrapper/vk_instance.cpp
    renderer/vulkan_wrapper/vk_instance.hpp)

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
    ${RENDERER_FILES}
    ${VULKAN_WRAPPER_FILES})

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

# Group the source files to keep the project nicely structured
source_group("main" FILES main.cpp)
source_group("miscellaneous" FILES ${MISCELLANEOUS_FILES})
source_group("renderer" FILES ${RENDERER_FILES})
source_group("vulkan" FILES ${VULKAN_WRAPPER_FILES})
