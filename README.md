# Graphics2

![Volumetric Rendering](/resources/screenshots/1.png)

### Description
OpenGL project for prototyping.
Contains abstractions for major OpenGL features with focus on code reusability, modern OpenGL usage and ease of use.
Also contains some demos and examples.

### OpenGL version and extensions:
* 4.5 and up should work
* [ARB_bindless_texture](https://www.khronos.org/opengl/wiki/Bindless_Texture)
* I'm trying to use [direct state access](https://www.khronos.org/opengl/wiki/Direct_State_Access) wherever possible

### Used libraries:
* [ImGui](https://github.com/ocornut/imgui)
* [stb_image & stb_image_write](https://github.com/nothings/stb)
* [GLshader](https://gitlab.uni-koblenz.de/johannesbraun/glshader) by Johannes Braun
* [Assimp](http://assimp.org/) (tested with 3.3.1)
* [glfw3](http://www.glfw.org/) (tested with 3.2.1)
* [glbinding](https://github.com/cginternals/glbinding) (tested with 2.1.1)
* [glm](https://glm.g-truc.net/0.9.8/index.html) (tested with 0.9.8.4)

### Usage
#### Windows
* Please use [vcpkg](https://github.com/Microsoft/vcpkg) for dependency management when using windows to use this project
* Install assimp:x64-windows, glfw3:x64-windows, glbinding:x64-windows and glm:x64-windows using vcpkg
* Set the correct path to your installation of vcpkg (vcpkg.cmake) in the CMakeSettings.json-file or use the included script (set_vcpkg_path.ps1) to select it
* Open the project folder in Visual Studio
#### Linux
* Currently not tested
* Install assimp, glfw3, glbinding and glm using your package manager
* Using CMake to generate makefiles might already work

### Executables
#### Project: Volumetric Lighting
* University Project for the Lecture 'Real-Time Rendering'
* Uses [this approach](http://advances.realtimerendering.com/s2014/wronski/bwronski_volumetric_fog_siggraph2014.pdf)


#### demo 1: cubemap, reflections, lighting
* work in progress
* phong lighting (spotlights, directional lights, specular & diffuse, fog)
* toon shading
* cubemap reflections
* postprocessing
* GUI-controllable parameters for basically everything in this demo
* shader live reloading

#### demo 2: distance fields
* work in progress
* raymarching distance fields: soft shadows, simple distance field ambient occlusion
* shader live reloading
* heavily inspired by http://www.iquilezles.org/, university lectures

#### demo 3: shaders
* work in progress
* testing some shader-only drawing stuff, "the book of shaders"-style

#### demo 4: shadowmapping
* work in progress
* shadow mapping with PCF

#### demo 5: PBR & IBL
* PBR using Cook-Torrance BRDF
* IBL using HDR cubemaps
* HDR texture -> cubemap -> specular & diffuse IBL

### Resources/Licensing

* PBR & IBL code adapted (with changes), skybox texture from [Joey de Vries](https://twitter.com/JoeyDeVriez)' [learnopengl.com](learnopengl.com), licensed under [CC BY-NC 4.0](https://creativecommons.org/licenses/by/4.0/legalcode).
* IBL Textures are from the [sIBL archive](http://www.hdrlabs.com/sibl/archive.html), licensed under [CC BY-NC-SA 3.0 US](https://creativecommons.org/licenses/by-nc-sa/3.0/us/legalcode).
