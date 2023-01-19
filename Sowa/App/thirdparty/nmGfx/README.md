# nmGfx
**nmGfx** is a rendering library with OpenGL backend with easy to use API.



## Usage
- Initializing renderer.
```cpp
    nmGfx::Renderer renderer;
    renderer.Init(
        1280, 720, "nmGFX",
        nmGfx::WindowFlags::NONE);
```

- Rendering
```cpp
    Model model;
    model.LoadFromFile("model.obj");
    
    // Rendering of 2D/3D/UI doesn't have to be in order since they are rendered to screen later
    while(!window.ShouldClose())
    {
        window.PollEvents();
    
        // Drawables are rendered on a seperate framebuffer and rendered to screen later
        renderer.Begin3D(projectionMatrix, cameraTransform);
        renderer.DrawModel(model, modelTransform, material);
        renderer.End3D();
        
        renderer.Begin2D(projectionMatrix, cameraTransform);
        renderer.DrawTexture(texture, textureTransform);
        renderer.End2D();

        // Renders fullscreen quad with 3D scene framebuffer texture
        renderer.Draw3DLayer();
        renderer.Draw2DLayer();

        window.SwapBuffers();
    }
```


## Building
nmGfx is a CMake project and building it is like any normal CMake project.

### Dependencies:
- [glfw](https://www.glfw.org/)

Running the following commands will generate:
- ```build/Playground``` executable for showcasing renderer capabilities if ```NMGFX_BUILD_PLAYGROUND``` is True
- ```build/libnmGfx.a``` static library that has all renderer source files
```
mkdir build
cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release -DNMGFX_BUILD_PLAYGROUND=True
cmake --build build/
```