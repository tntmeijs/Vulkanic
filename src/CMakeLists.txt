set(RENDERER_FILES
    renderer/renderer.cpp
    renderer/renderer.hpp)

# Find Vulkan on the system
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} main.cpp ${RENDERER_FILES})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

# Group the source files to keep the project nicely structured
source_group("main" FILES main.cpp)
source_group("renderer" FILES ${RENDERER_FILES})
