cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME Vulkanic)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(${PROJECT_NAME})

# Find Vulkan on the system
find_package(Vulkan REQUIRED)

# Use the Vulkan Windows extension
if(MSVC)
	add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
endif(MSVC)

# Vulkanic
add_subdirectory(src)
target_include_directories(${PROJECT_NAME} PRIVATE src)

# All third-party dependencies
add_subdirectory(third_party)

# Use C++17
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

if(MSVC)
    # Maximum warning level and treat warnings as errors
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)

    # Set the path tracer project as the Visual Studio startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

	# Set the debugging working directory
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
else(MSVC)
    # Maximum warning level and treat warnings as errors
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif(MSVC)
