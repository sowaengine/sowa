#ifdef NMGFX_BUILD_PLAYGROUND

#define NMGFX_PRINT_MESSAGES

#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// glad C/C++ Debug 3.3 core profile

#include "glm/gtc/matrix_transform.hpp"

#include "Core/nm_Renderer.hpp"
#include "Core/GL/nm_Model.hpp"
#include "Core/GL/nm_Material.hpp"
#include "Core/nm_Matrix.hpp"

int main(int argc, char const *argv[])
{
    nmGfx::Renderer renderer;
    renderer.Init(
        1280, 720, 1920, 1080, "nmGFX",
        nmGfx::WindowFlags::NONE);

    nmGfx::Window& window = renderer.GetWindow();


    renderer.GetData2D()._shader.LoadFile("res/default2d.glsl");
    renderer.GetData3D()._shader.LoadFile("res/default.glsl");
    renderer.GetData3D()._skyboxShader.LoadFile("res/skybox.glsl");
    renderer.GetDataFullscreen()._shader.LoadFile("res/fullscreen.glsl");
    
    nmGfx::Model model;
    model.LoadFromFile("res/viking_room.obj");

    std::shared_ptr<nmGfx::Texture> tex = std::make_shared<nmGfx::Texture>();
    tex->Load2DFromFile("res/viking_room.png");

    std::shared_ptr<nmGfx::Texture> tex2d = std::make_shared<nmGfx::Texture>();
    tex2d->Load2DFromFile("res/planet09.png");

    nmGfx::Material mat;
    mat.albedo = {0.2f, 0.7f, 0.1f, 1.f};
    mat.albedo_tex = tex;
    float t = 0.f;
    while(!window.ShouldClose())
    {
        window.PollEvents();
        if(window.IsKeyPressed(nmGfx::Window::Key::Key_K))
            std::cout << "Key 'K' is pressed" << std::endl;
        if(!window.IsWindowHovered())
            std::cout << "Cursor is out of window" << std::endl;
        if(window.IsMouseButtonPressed(nmGfx::Window::MouseButton::LEFT))
            std::cout << "'Mouse Button Left' is pressed" << std::endl;

        glm::mat4 camera = glm::translate(glm::mat4(1.f), {0.f, 0.5f, 2.f});

        renderer.Begin3D(nmGfx::CalculatePerspective((float)window.GetVideoWidth() / (float)window.GetVideoHeight(), 60.f, 0.1f, 500.f), camera);
        renderer.DrawModel(model, nmGfx::CalculateModelMatrix({0.f, 0.f, 0.f}, {90.f, 180.f, 0.f}, {1.f, 1.f, 1.f}), mat, 12);
        // printf("3D ID: %i\n", renderer.Get3DPickID(window.GetVideoMousePosX(), window.GetVideoHeight() - window.GetVideoMousePosY()));
        renderer.End3D();

        
        renderer.Begin2D(nmGfx::CalculateModelMatrix({0.f, 0.f, 1.f}, {.0f, .0f, t*10}, {1.f, 1.f, 1.f}));
        renderer.DrawTexture(tex2d.get(), nmGfx::CalculateModelMatrix({-200.f, 0.f}, 0, {tex2d->GetWidth() * 0.3f, tex2d->GetHeight() * 0.3f}), {0.f, 1.f, 1.f}, 13);
        renderer.DrawTexture(tex2d.get(), nmGfx::CalculateModelMatrix({200.f, 0.f}, 0, {tex2d->GetWidth() * 0.3f, tex2d->GetHeight() * 0.3f}), {1.f, 0.f, 1.f}, 14);

        // printf("2D ID: %i\n", renderer.Get2DPickID(window.GetVideoMousePosX(), window.GetVideoHeight() - window.GetVideoMousePosY()));
        renderer.End2D();

        t += 0.01f;

        renderer.ClearLayers();
        renderer.Draw3DLayer();
        renderer.Draw2DLayer();

        window.SwapBuffers();
    }
    

    return 0;
}

#endif