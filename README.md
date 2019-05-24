# Vulkanic
A 3D renderer built using Vulkan for Windows (may support additional platforms in the future).

# Requirements
- CMake 3.14
- Visual Studio 2017 **or** Visual Studio 2019

**Make sure you add CMake to your PATH if you want to follow the instructions below!**

# Installing
1. Open a terminal window
2. Navigate to the root folder of the repository
3. Run the following commands (Windows):
    1. `git submodule update --init`
    2. `mkdir build`
    3. `cd build`
    4. **Visual Studio 2017:** `cmake .. -G "Visual Studio 15 2017" -A x64`<br>
       **Visual Studio 2019:** `cmake .. -G "Visual Studio 16 2019" -A x64`
