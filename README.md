# RHI
Graphic library provides generic independent API for rendering.
Provided backends:
- Vulkan (in progress)
- DirectX (in plan)
- Metal (in plan)
## How to integrate and build
This project uses CMake as build system, so you can just download this project into folder and include it in your CMakeLists via `add_subdirectory`
You can link it to another project via `target_link_libraries(${target_name} PRIVATE RHI)`
Also you can define some options:
- RHI_VULKAN_BACKEND - RHI will be compiled with Vulkan backend.
- RHI_DIRECTX_BACKEND - RHI will be compiled with DirectX backend.
- RHI_METAL_BACKEND -RHI will be compiled with Metal backend
- RHI_BUILD_EXAMPLES - example applications from Example folder will be compiled
### Dependencies
The library provides independent API, so you don't have to link any other libraries. Backends have their own dependencies which are intergrated with FetchContent

## How to use
There is only one header file you need to include in code for using - RHI.hpp. It contains all interface declaration you can use to write code.


