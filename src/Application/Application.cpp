/**
 * @file Application.cpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#include "Application.hpp"
#include "Window.hpp"
#include "glad/glad.h"

#include <EaseGL.hpp>
#include "Editor/Editor.hpp"

#include <iostream>

#include "EaseEngine.hpp"

#include "Scene/Scene.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Components.hpp"

#include "Renderer/Renderer.hpp"


Application::Application()
{
}

Application::~Application()
{
}


Application& Application::get_singleton() 
{
   static Application app;
   return app;
}

void Application::Run() 
{
   // Get Project
   Project& project = Project::get_singleton();
   ProjectSettings& projectSettings = project.projectSettings;

   // Initialize window
   assert(glfwInit() == GLFW_TRUE && "Couldn't Initialize GLFW");
   glfwWindowHint(GLFW_SAMPLES, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   
   Window& window = Window::get_singleton();
   window.CreateWindow(
      projectSettings.WindowSize.x,
      projectSettings.WindowSize.y,
      projectSettings.Title);
   
   assert(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) && "Couldn't Initialize GLAD");
   glDisable(GL_CULL_FACE);
   //glEnable(GL_DEPTH_TEST);
   glEnable(GL_STENCIL_TEST);
   glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

   CreateFinalFramebuffer();   
   

   Renderer& renderer = Renderer::get_singleton();
   renderer.Init();


   Editor& editor = Editor::get_singleton();
   editor.m_EditorData = &m_EditorData;
   EASE_LOG_VERSION();


   Scene mainScene;
   Scene runtimeScene;
   m_CurrentScene = &mainScene;
   m_EditorData.currentScene = m_CurrentScene;


   glm::vec2 editor2DCameraPos = {0.0f, 0.0f};


   EaseGL::GLTexture tex = EaseGL::Texture2D::New(project.GetAbsolutePath("sample.png").c_str());


   Entity camEntity = mainScene.Create("Camera");
   auto& cam = camEntity.addComponent<CameraComponent>();
   cam.current = true;
   camEntity.addComponent<Transform2DComponent>();

   for(int i=0; i<2; i++)
   {
      Entity entity = mainScene.Create("Entity" + std::to_string(i));
      auto& tc = entity.addComponent<Transform2DComponent>();
      tc.position = glm::vec2(i * 150 + 100, 300);
      tc.scale = glm::vec2(1.0f, 1.0f);
      tc.rotation = (i+1)*10;

      auto& sprite = entity.addComponent<SpriteComponent>();
      sprite.ptexture = &tex;
   }

   
   
   window.SetClearColor(0.2f, 0.2f, 0.2f, 1.0f);
   // Main Loop
   while (!window.WindowShouldClose())
   { 
      m_FinalFramebuffer.Bind();
      window.Clear(GL_COLOR_BUFFER_BIT);// | GL_DEPTH_BUFFER_BIT);
      

      glm::mat4 proj2d = glm::ortho(0.0f, (float)window.Width(), 0.0f, (float)window.Height(), 0.01f, 11.1f); // projection matrix used in 2d rendering
      glm::mat4 camera2dTransform = glm::translate(glm::mat4(1.0f), {editor2DCameraPos.x, editor2DCameraPos.y, 1.0f}); // default 2d camera z is 1.0f
      // Find current camera
      if(m_EditorData.useInGameCamera)
      {
         for(const auto& e : m_CurrentScene->GetRegistry().view<Transform2DComponent, CameraComponent>())
         {
            Entity entity(e, &m_CurrentScene->GetRegistry());
            if(entity.getComponent<CameraComponent>().current)
            {
               camera2dTransform = entity.getComponent<Transform2DComponent>().GetTransform();
               camera2dTransform = glm::translate(camera2dTransform, {-window.Width()*0.5f, -window.Height()*0.5f, 1.0f});
               break;
            }
         }
      }

      // Draw Here
      renderer.Begin2D();
      for(const auto& e : m_CurrentScene->GetRegistry().view<Transform2DComponent, SpriteComponent>())
      {
         Entity entity(e, &m_CurrentScene->GetRegistry());
         EaseGL::GLTexture* ptexture = entity.getComponent<SpriteComponent>().ptexture;
         
         glm::mat4 model = entity.getComponent<Transform2DComponent>().GetTransform(glm::vec2(ptexture->Width(), ptexture->Height()));
         
         // if currently drawn entity is the one that is selected in the editor,
         // first draw it a little bigger on behind to make it look outlined (temporary solution)
         if(entity == m_EditorData.selectedEntity)
         {
            renderer.Get2DShader().Uniform("outline", 1);
            renderer.DrawSprite(glm::scale(model, {1.05f, 1.05f, 1.0f}), camera2dTransform, proj2d, ptexture);
            renderer.Get2DShader().Uniform("outline", 0);
         }
         
         renderer.DrawSprite(model, camera2dTransform, proj2d, ptexture);
      }

      m_FinalFramebuffer.Unbind();



      m_EditorFramebuffer.Bind();
      m_FramebufferVertexArray->Bind();
      m_FinalFramebufferShader.Bind();
      m_FinalFramebufferShader.Uniform("tex",
               EaseGL::GLTexture(m_FinalFramebuffer.GetColorAttachment(0),
               EaseGL::TextureType::TEXTURE2D), 0);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      m_EditorFramebuffer.Unbind();

      

      editor.Update();

      if(!m_EditorData.useInGameCamera)
      {
         editor2DCameraPos.x += m_EditorData.viewportDragMouseDeltaX;
         editor2DCameraPos.y += m_EditorData.viewportDragMouseDeltaY;
      }
      

      if(m_EditorData.start_stop_pressed)
      {
         static bool oldState_useInGameCamera; // saves useInGameCamera state when game started, and reverts back to it when game stopped
         if(m_Running)
         {
            editor.SetStyle(0);
            m_CurrentScene = &mainScene;
            m_EditorData.currentScene = m_CurrentScene;
            m_EditorData.selectedEntity.SetRegistry(&m_CurrentScene->GetRegistry());

            if(!oldState_useInGameCamera)
               m_EditorData.useInGameCamera = false;
            Stop();
         }
         else
         {
            editor.SetStyle(1);
            m_CurrentScene = &runtimeScene;
            m_EditorData.currentScene = m_CurrentScene;
            Scene::CopyScene(mainScene, runtimeScene);
            m_EditorData.selectedEntity.SetRegistry(&m_CurrentScene->GetRegistry());

            oldState_useInGameCamera = m_EditorData.useInGameCamera;
            m_EditorData.useInGameCamera = true;
            Start();
         }

         m_Running = !m_Running;
         m_EditorData.start_stop_pressed = false;
      }
      

      window.SwapBuffers();
      window.PollEvents();
   }

}

void Application::Start()
{
   Editor::get_singleton().Log << "App Started" << Log::Endl;
}

void Application::Stop()
{
   Editor::get_singleton().Log << "App Stopped" << Log::Endl;
}


void Application::CreateFinalFramebuffer()
{
   m_FinalFramebufferShader.LoadShader(Project::get_singleton().GetAbsolutePath("framebuffer.glsl").c_str());

   {
      EaseGL::FrameBufferCreateInfo createInfo;
      createInfo.width = Window::get_singleton().Width();
      createInfo.height = Window::get_singleton().Height();
      createInfo.attachments = {
            EaseGL::FramebufferAttachmentFormat::RGB,
            EaseGL::FramebufferAttachmentFormat::DEPTH24STENCIL8 };
      m_FinalFramebuffer.RecreateBuffer(createInfo);
   }
   {
      EaseGL::FrameBufferCreateInfo createInfo;
      createInfo.width = Window::get_singleton().Width();
      createInfo.height = Window::get_singleton().Height();
      createInfo.attachments = {
            EaseGL::FramebufferAttachmentFormat::RGB,
            EaseGL::FramebufferAttachmentFormat::DEPTH24STENCIL8 };
      m_EditorFramebuffer.RecreateBuffer(createInfo);
   }
   m_EditorData.finalFramebufferTextureID = m_EditorFramebuffer.GetColorAttachment(0);


   float frame_vertices[] = {
         1.0f,  1.0f, 0.0f,    1.0f, 1.0f,  // top right
         1.0f, -1.0f, 0.0f,    1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,  // bottom left
        -1.0f,  1.0f, 0.0f,    0.0f, 1.0f,  // top left 
   };
   unsigned int indices[] = {  // note that we start from 0!
      0, 1, 3,   // first triangle
      1, 2, 3    // second triangle
   };

   m_FramebufferVertexArray = std::make_unique<EaseGL::VertexArray>();

   static EaseGL::GLBuffer frame_vertexBuffer = EaseGL::VertexBuffer::New();
   frame_vertexBuffer.BufferData(frame_vertices, sizeof(frame_vertices), EaseGL::GLBufferUsage::STATIC_DRAW);

   static EaseGL::GLBuffer frame_indexBuffer = EaseGL::IndexBuffer::New();
   frame_indexBuffer.BufferData(indices, sizeof(indices), EaseGL::GLBufferUsage::STATIC_DRAW);
   
   m_FramebufferVertexArray->SetAttribute(0, EaseGL::AttributeType::VEC3);
   m_FramebufferVertexArray->SetAttribute(1, EaseGL::AttributeType::VEC2);
   m_FramebufferVertexArray->UploadAttributes();

   EaseGL::VertexArray::Unbind();
}