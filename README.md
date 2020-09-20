# Vulkan renderer

This is a small application that uses the Vulkan API to render 3D models stored in PLY format. One place where suitable models can be found is the [Stanford 3D Scanning Repository](https://graphics.stanford.edu/data/3Dscanrep/). The app can be built using Visual Studio 2019 and was tested on a mobile RTX 2070.

Models can be rotated by dragging the mouse, while the distance between camera and model can be changed using the mouse wheel.

## Dependencies
* [cxxopts](https://github.com/jarro2783/cxxopts/) (included as submodule)
* [LodePNG](http://lodev.org/lodepng/) (must be installed through vcpkg)
* [LunarG Vulkan SDK](https://vulkan.lunarg.com/) (must be installed)
* [GLFW](http://www.glfw.org/) (must be installed through vcpkg)
* [GLM](http://glm.g-truc.net/) (included as submodule)
* [ImGui](https://github.com/ocornut/imgui/) (must be installed through vcpkg)
* [tiny file dialogs](https://sourceforge.net/projects/tinyfiledialogs/) (must be installed through vcpkg)
* [tinyply](https://github.com/ddiakopoulos/tinyply/) (included as submodule)

## Screenshots

Stanford Armadillo model ([source](https://graphics.stanford.edu/data/3Dscanrep/), coloured using [MeshLab](http://www.meshlab.net/))

![Armadillo](Screenshots/armadillo.png)

Stanford Lucy model ([source](https://graphics.stanford.edu/data/3Dscanrep/))

![Lucy](Screenshots/lucy.png)
