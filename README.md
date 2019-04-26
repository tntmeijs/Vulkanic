# Vulkanic
A 3D renderer built using Vulkan for Windows (may support additional platforms in the future).

# Requirements
- Python 3.6
- CMake 3.14
- Visual Studio 2017 **or** Visual Studio 2019

**Make sure you add CMake and Python to your PATH!**

# Installing
1. Open a terminal window
2. Navigate to the root folder of the repository
3. Run the following commands (Windows):
    1. `mkdir build`
    2. `cd build`
    3. **Visual Studio 2017:** `cmake .. -G "Visual Studio 15 2017" -A x64`<br>
       **Visual Studio 2019:** `cmake .. -G "Visual Studio 16 2019" -A x64`
