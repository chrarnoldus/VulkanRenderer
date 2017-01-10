# Vulkan renderer

This is a small application that uses the Vulkan API to render 3D models stored in PLY format. One place where suitable models can be found is the [Stanford 3D Scanning Repository](https://graphics.stanford.edu/data/3Dscanrep/). The purpose of this application is to try out the Vulkan API; it is probably not a good display of how to design such an application and there are likely violations of the Vulkan specification as well.

Building was only tested using Visual Studio 2015. Running was only tested using a GTX 860M.

Models can be rotated by dragging the mouse, while the distance between camera and model can be changed using the mouse wheel. The application has an ImGui-based UI, but it does not do anything useful yet.

## Dependencies
* [LunarG Vulkan SDK](https://vulkan.lunarg.com/) (must be installed)
* [GLFW](http://www.glfw.org/) (included as submodule)
* [GLM](http://glm.g-truc.net/) (included as submodule)
* [ImGui](https://github.com/ocornut/imgui/) (included as submodule)
* [tiny file dialogs](https://sourceforge.net/projects/tinyfiledialogs/) (included as submodule)
* [tinyply](https://github.com/ddiakopoulos/tinyply/) (included as submodule)

## Screenshots

Stanford Armadillo model ([source](https://graphics.stanford.edu/data/3Dscanrep/), coloured using [MeshLab](http://www.meshlab.net/))

![Armadillo](Screenshots/armadillo.png)

Stanford Lucy model ([source](https://graphics.stanford.edu/data/3Dscanrep/))

![Lucy](Screenshots/lucy.png)
