set(SOURCES main.cpp)

# Find Vulkan on the system
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
