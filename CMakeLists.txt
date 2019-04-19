cmake_minimum_required(VERSION 3.14)
set(PROJECT_NAME Vulkanic)

project(${PROJECT_NAME})

add_subdirectory(src)
add_subdirectory(third_party)

# Use C++17
set_target_properties(${RENDERER_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
set_target_properties(${PATH_TRACER_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

if(MSVC)
    # Warning level 3 (GLM causes issues otherwise) and treat warnings as errors on MSVC
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)

    # Set the path tracer project as the Visual Studio startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else(MSVC)
    # Maximum warning level and treat warnings as errors
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif(MSVC)
