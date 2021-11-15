#include "Application.hpp"

#include <dlfcn.h>

#include "Ease.hpp"
#include "Window/Window.hpp"
#include "Renderer/gl/glRenderer.hpp"

#include "Global.hpp"

#include "Renderer/Texture/Texture.hpp"
#include "Renderer/Material/Material.hpp"

#include "Renderer/Framebuffer/GLFramebuffer.hpp"

#include "Serializer/Serializer.hpp"

#include "Lang/Lua/luaScript.hpp"
#include "Lang/Lua/Api/API.hpp"

#include "EaseEngine.hpp"

#include "Utils/File.hpp"

#include "Scene/Components.hpp"
#define EASE_EDITOR
#ifdef EASE_EDITOR
   #include "Editor/Editor.hpp"
#endif

#include "Utils/Matrix.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/vector_angle.hpp"

/**
 * TODO:
 * Editor CameraComponent
 * Scene Deserialization
 * Scene Serialization
 * Copying scene for runtime (copy scene to temporary scene tree when started, reload the scene(revert back to editor stage) when stopped)
 * 
*/


namespace Ease
{

SceneTree Global::sceneTree;
Project* Global::project;


void Application::Run()
{
   Debug::test_and_throw(glfwInit(), "Cannot initialize glfw");
   m_Window.create(1280, 720, "Ease Engine");
   // Pass window to Renderer
   GLRenderer::setWindow(&m_Window);
   
   glClearColor(0.25, 0.25, 0.25, 1.0);
   std::cout << "GL version: " << glGetString(GL_VERSION) << std::endl;

#pragma region initialize project
   Project project;
   Serializer::DeserializeProject("testproj/project.ease", project);
   Global::project = &project;

   m_Window.setTitle(project.projectSettings.title);
   m_Window.setVideoSize(project.projectSettings.renderWidth, project.projectSettings.renderHeight);
   m_Window.setWindowSize(project.projectSettings.windowWidth, project.projectSettings.windowHeight);
   m_Window.resizeWindow();
#pragma endregion


#pragma region Editor Camera Setup
   //Global::sceneTree = SceneTree();
   m_EditorCamera2DTransform = Transform3DComponent();
   m_EditorCamera2DTransform.translation = glm::vec3(0, 0, 0);
   m_EditorCamera2DTransform.scale = glm::vec3(1, 1, 1);
   m_EditorCamera2DTransform.rotation = {0.f, 0.f, 0.f};
   m_EditorCamera2D = CameraComponent();
   m_EditorCamera2D.projectionType = CameraProjection::ORTHOGRAPHIC;
   
   m_EditorCamera3DTransform = Transform3DComponent();
   m_EditorCamera3DTransform.translation = {0.f, 300.f, 100.f};
   m_EditorCamera3DTransform.scale = {128.f, 128.f, 128.f};
   m_EditorCamera3DTransform.rotation = {0.f, 0.f, 0.f};

   m_EditorCamera3D = CameraComponent();
   m_EditorCamera3D.projectionType = CameraProjection::PERSPECTIVE;
   m_EditorCamera3D.fov = glm::radians(60.f);
   m_EditorCamera3D.aspect = 16.f / 9.f;
   m_EditorCamera3D.near = 0.1f;
   m_EditorCamera3D.far = 1000.f;
#pragma endregion

#pragma region Global Models & Shaders Setup
   // setup post process shader
   m_PostProcessShader.createShader("res://shaders/postprocess.vert","res://shaders/postprocess.frag");

   // setup sprite2d model
   m_Sprite2DShader.createShader("res://shaders/shader.vert", "res://shaders/shader.frag");
   m_Sprite2DShader.bind();

   m_Sprite2DModel = std::make_shared<Model>();
   m_Sprite2DModel->setVertices({
      { {-0.5f, -0.5f, 0.0f}, { 0.0f, 0.0f } },
      { {-0.5f,  0.5f, 0.0f}, { 0.0f, 1.0f } },
      { { 0.5f,  0.5f, 0.0f}, { 1.0f, 1.0f } },
      { { 0.5f, -0.5f, 0.0f}, { 1.0f, 0.0f } },
   });
   m_Sprite2DModel->setIndices({
      0, 1, 2,
      2, 3, 0
   });
   m_Sprite2DModel->setShader(&m_Sprite2DShader);
   m_Sprite2DModel->recreateModel();

   GLFramebuffer framebuffer;
   framebuffer.recreateFrameBuffer();
#pragma endregion


// initialize editor
#ifdef EASE_EDITOR
   Editor editor;
   editor.editorData.finalFrameBuffer = &framebuffer;
   editor.editorData.window = &m_Window;
   editor.editorData.camera2DTransform = &m_EditorCamera2DTransform;
   editor.editorData.camera3DTransform = &m_EditorCamera3DTransform;
   editor.editorData.editorCamera2D = &m_EditorCamera2D;
   editor.editorData.editorCamera3D = &m_EditorCamera3D;

   editor.init();

#else
   startGame();
#endif
   
   
   using namespace Comp;
   // cl_Stats is the class to monitor stuff & debug (like draw calls etc.)
   cl_Stats _stats;
   stats = &_stats;


   std::shared_ptr<Texture> tex = std::make_shared<Texture>();
   tex->setFilePath("res://textures/ship.png");
   tex->recreateTexture();
   
   std::shared_ptr<Material> material = std::make_shared<Material>();
   material->albedoTexture = tex;
   material->color = {0.1, 0.4, 0.6, 1.0};

   std::shared_ptr<Material> material2 = std::make_shared<Material>();
   material2->albedoTexture = tex;
   material2->color = {0.1, 0.2, 0.1, 1.0};

   m_FinalFramebufferNode = Global::sceneTree.createNode("finalFrameBuffer Entity");
   auto& framebufferTransform = m_FinalFramebufferNode->addComponent<Transform3DComponent>();
   auto& framebufferMesh = m_FinalFramebufferNode->addComponent<MeshComponent>();
   
   std::shared_ptr<Material> f_material = std::make_shared<Material>();
   f_material->albedoTexture = std::make_shared<Texture>(framebuffer.getColorAttachment(), framebuffer.createInfo.width, framebuffer.createInfo.height, 3);
   f_material->color = {1.0, 1.0, 1.0, 1.0};

   framebufferMesh.model = m_Sprite2DModel;
   framebufferMesh.material = f_material;
   framebufferMesh.Visible = false;

   Node* node5;
   for(int x=0; x<10; x++)
   {
      Node* node = Global::sceneTree.createNode( std::to_string(x) );
      node->addComponent<Transform3DComponent>( glm::vec3(x*140 + 100, 300.0f, 0.f));
      auto& mesh = node->addComponent<MeshComponent>();
      mesh.model = m_Sprite2DModel;
      mesh.material = material;
      mesh.Visible = true;

      Global::sceneTree.getRoot()->addChild(node);

      if(x == 5)
         node5 = node;
   }
   Node* node = Global::sceneTree.createNode( "Children" );
   node->addComponent<Transform3DComponent>( glm::vec3(250, 400, 0.f));
   auto& mesh = node->addComponent<MeshComponent>();
   mesh.model = m_Sprite2DModel;
   mesh.material = material;
   mesh.Visible = true;

   node5->addChild(node);

   LuaScript testScript{};
   testScript.loadScript("res://scripts/testScript.lua");

   Node* scriptNode = Global::sceneTree.createNode("TestScript Object");
   auto& scriptBehaviour = scriptNode->addComponent<LuaBehaviour>();
   scriptBehaviour.script = testScript;
   //auto& nativeBehaviour = scriptNode->addComponent<NativeBehaviour>();
   //nativeBehaviour.sharedObjectPath = File::resolveFilepath("res://scripts/test.so").fullPath;
   



   glEnable(GL_DEPTH_TEST);
   // Main Loop
   while (true)
   {
      stats->drawCalls = 0;
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if(isRunning)
         updateGame();
      
      framebuffer.bind();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // Draw Here
      {
         if(editor.editorData.editorCameraType == EditorCameraType::TWO_D)
            GLRenderer::drawModels(false, m_EditorCamera2DTransform, m_EditorCamera2D);
         else
            GLRenderer::drawModels(false, m_EditorCamera3DTransform, m_EditorCamera3D);
      }
      framebuffer.unbind();


#ifdef EASE_EDITOR
      editor.beginDraw();
      editor.editorData.gameRunning = isRunning;
      
      EditorReturnData editorReturnData = editor.update();

      // draw your imgui stuff here
      if(editorReturnData.gameStartResult == EditorGameStartResult::START)
      {
         startGame();
      }
      else if(editorReturnData.gameStartResult == EditorGameStartResult::STOP)
      {
         stopGame();
      }

      { // Set 2D editor camera size to video size
         int w, h;
         m_Window.getVideoSize(&w, &h);
         m_EditorCamera2D.size = {w, h};
      }

      if(editorReturnData.editorCameraType == EditorCameraType::TWO_D)
      {
         m_EditorCamera2DTransform.translation.x -= editorReturnData.viewportDragDelta.x*m_EditorCamera2DTransform.scale.x;
         m_EditorCamera2DTransform.translation.y -= editorReturnData.viewportDragDelta.y*m_EditorCamera2DTransform.scale.y;

         float scroll = editorReturnData.viewportScrollDelta.x*0.1;
         m_EditorCamera2DTransform.scale.x -= scroll;
         m_EditorCamera2DTransform.scale.y -= scroll;
      } else
      {
         
         if(ImGui::IsMouseDown(ImGuiMouseButton_Right))
         {
            m_EditorCamera3DTransform.rotation.y -= glm::radians(editorReturnData.viewportDragDelta.x) * 0.2f;
            m_EditorCamera3DTransform.rotation.x += glm::radians(editorReturnData.viewportDragDelta.y) * 0.2f;
         }

         if(glfwGetKey(m_Window.get(), GLFW_KEY_W))
         {
            //glm::mat4 transform = m_EditorCamera3DTransform.getTransform();
            //transform = glm::translate(transform, glm::vec3{0.f, 0.f, -1.f}*0.1f );
            
            //transform = glm::inverse(glm::lookAt(m_EditorCamera3DTransform.translation, glm::vec3(transform[3]), {0.f, 1.f, 0.f}));

            //glm::vec3 tempScale{128.f};
            //Matrix::decomposeMat4(transform, m_EditorCamera3DTransform.translation, m_EditorCamera3DTransform.rotation, tempScale);
         }

         
      }

      ImGui::DragFloat3("Pos", &m_EditorCamera3DTransform.translation.x);
      //ImGui::DragFloat3("Rot", &m_EditorCamera3DTransform.rotation.x);
      ImGui::SliderAngle("RotX", &m_EditorCamera3DTransform.rotation.x);
      ImGui::SliderAngle("RotY", &m_EditorCamera3DTransform.rotation.y);
      ImGui::SliderAngle("RotZ", &m_EditorCamera3DTransform.rotation.z);
      ImGui::DragFloat3("Scale", &m_EditorCamera3DTransform.scale.x);

      editor.endDraw();
#else
      drawFinalFramebuffer();
#endif
      

      glfwSwapBuffers(m_Window.get());

      
      glfwPollEvents();
      if(m_Window.shouldClose())
         break;
   }

   glfwTerminate();
}

void Application::drawFinalFramebuffer()
{
   using namespace Global;

   auto& transform = m_FinalFramebufferNode->getComponent<Transform3DComponent>();
   auto& mesh = m_FinalFramebufferNode->getComponent<MeshComponent>();

   int w, h;
   m_Window.getWindowSize(&w, &h);
   
   transform.translation = {w/2, h/2, 0.f};
   transform.scale = {w, h, 1.f};

   


   
   
   
   mesh.Visible = true;
   glDisable(GL_DEPTH_TEST);
   GLRenderer::drawMesh(&m_PostProcessShader, transform, mesh);
   glEnable(GL_DEPTH_TEST);
   mesh.Visible = false;
}


void Application::startGame()
{
   if(isRunning) return;
   isRunning = true;
   
   using namespace Comp;

#pragma region LuaBehaviour
   g_LuaState = luaL_newstate();
   // bind global lua functions here
   luaL_openlibs(g_LuaState);

   
   // get_node
   lua_pushcfunction(g_LuaState, Lua::Node::getNode);
   lua_setglobal(g_LuaState, "get_node");

   lua_pushcfunction(g_LuaState, Lua::Vector3::create);
   lua_setglobal(g_LuaState, "Vector3");
   

   
   // Metatables
   luaL_newmetatable(g_LuaState, "NodeMetaTable");
   lua_pushstring(g_LuaState, "__index");
   lua_pushcfunction(g_LuaState, Lua::Node::__index);
   lua_settable(g_LuaState, -3);
   lua_pushstring(g_LuaState, "__newindex");
   lua_pushcfunction(g_LuaState, Lua::Node::__newindex);
   lua_settable(g_LuaState, -3);
   
   luaL_newmetatable(g_LuaState, "Vector3MetaTable");
   lua_pushstring(g_LuaState, "__add");
   lua_pushcfunction(g_LuaState, Lua::Vector3::__add);
   lua_settable(g_LuaState, -3);

   luaL_newmetatable(g_LuaState, "TransformComponentMetaTable");
   lua_pushstring(g_LuaState, "__index");
   lua_pushcfunction(g_LuaState, Lua::Transform3D::__index);
   lua_settable(g_LuaState, -3);
   lua_pushstring(g_LuaState, "__newindex");
   lua_pushcfunction(g_LuaState, Lua::Transform3D::__newindex);
   lua_settable(g_LuaState, -3);


   // dofile on all scripts
   for(auto entity: Global::registry.view<LuaBehaviour>()) {
      auto& luaBehaviour = Global::registry.get<LuaBehaviour>(entity);
      luaBehaviour.script.loadScript(luaBehaviour.script.getFilename());
      if(luaBehaviour.script.isValid()) {
         //luaL_loadstring(g_LuaState, luaBehaviour.script.getScript().c_str());
         //lua_pcall(g_LuaState, 0, LUA_MULTRET, 0);
         int error = luaL_dostring(g_LuaState, luaBehaviour.script.getScript().c_str());
         if(error)
         {
            Editor::printLine(std::string("LuaBehaviour Error at: ") + (luaBehaviour.script.getFilename()) + "::" + lua_tostring(g_LuaState, -1));
            stopGame();
            return;
         }
         
         // call init
         lua_getglobal(g_LuaState, "init");
         lua_pcall(g_LuaState, 0, 1, 0);

         //init:Start
         lua_pushstring(g_LuaState, "Start");
         lua_gettable(g_LuaState, -2);
         // save the function if Start is a function
         if(lua_isfunction(g_LuaState, -1))
            luaBehaviour.startRef = luaL_ref(g_LuaState, LUA_REGISTRYINDEX);
         

         // init:Update
         lua_pushstring(g_LuaState, "Update");
         lua_gettable(g_LuaState, -2);
         // save the function if Update is a function
         if(lua_isfunction(g_LuaState, -1))
            luaBehaviour.updateRef = luaL_ref(g_LuaState, LUA_REGISTRYINDEX);
      }
   }

   // call start functions on all LuaBehaviour components
   for(auto entity: Global::registry.view<LuaBehaviour>()) {
      auto& luaBehaviour = Global::registry.get<LuaBehaviour>(entity);
      if(luaBehaviour.script.isValid())
      {
         if(luaBehaviour.startRef != -1)
         {
            // Call Start functions
            lua_rawgeti(g_LuaState, LUA_REGISTRYINDEX, luaBehaviour.startRef);
            if(lua_isfunction(g_LuaState, -1))
            {
               if( lua_pcall(g_LuaState, 0, 0, 0) != LUA_OK)
               {
                  Editor::printLine(std::string("Error: ") + lua_tostring(g_LuaState, -1));
                  lua_close(g_LuaState);
               }
               
            }
         }
      }
   }
#pragma endregion


#pragma region NativeBehaviour
   for(auto entity : Global::registry.view<NativeBehaviour>()) { break;
      auto& nativeBehaviour = Global::registry.get<NativeBehaviour>(entity);

      std::shared_ptr<NativeScriptResource> resource = std::make_shared<NativeScriptResource>();
      resource->loadScript(nativeBehaviour.sharedObjectPath.c_str());

      nativeBehaviour.resource = resource;

      create_t* create_triangle = (create_t*) dlsym(resource->getObj(), "create");
      const char* dlsym_error = dlerror();
      if (dlsym_error) {
         std:: cout << "Cannot load symbol create: " << dlsym_error << '\n';
         throw;
      }
      
      // create an instance of the class
      nativeBehaviour.obj = create_triangle();
      // use the class
      nativeBehaviour.obj->Start();

      
      // unload the triangle library
      //dlclose(obj);
   }
#pragma endregion


   Debug::log("Started Game");
}


void Application::updateGame()
{
   // disable on runtime builds
   if(!isRunning) return;


#pragma region LuaBehaviour
   // call start functions on all LuaBehaviour components
   for(auto entity: Global::registry.view<LuaBehaviour>()) {
      auto& luaBehaviour = Global::registry.get<LuaBehaviour>(entity);
      if(luaBehaviour.script.isValid())
      {
         if(luaBehaviour.updateRef != -1)
         {
            lua_pushnumber(g_LuaState, 1.0f / 120.f);
            lua_setglobal(g_LuaState, "delta");

            // Call Start functions
            lua_rawgeti(g_LuaState, LUA_REGISTRYINDEX, luaBehaviour.updateRef);
            if(lua_isfunction(g_LuaState, -1))
            {
               if( lua_pcall(g_LuaState, 0, 0, 0) != LUA_OK)
               {
                  Editor::printLine(std::string("Error: ") + lua_tostring(g_LuaState, -1));
               }
            }
         }
      }
   }
#pragma endregion

#pragma region NativeBehaviour
   for(auto entity : Global::registry.view<NativeBehaviour>()) { break;
      auto& nativeBehaviour = Global::registry.get<NativeBehaviour>(entity);

      nativeBehaviour.obj->Update();
   }
#pragma endregion


}

void Application::stopGame()
{
   if(!isRunning) return;
   isRunning = false;

   // LuaScript
   lua_close(g_LuaState);

   // Native
   for(auto entity : Global::registry.view<NativeBehaviour>()) { break;
      auto& nativeBehaviour = Global::registry.get<NativeBehaviour>(entity);


      destroy_t* destroy_triangle = (destroy_t*) dlsym(nativeBehaviour.resource->getObj(), "destroy");
      const char* dlsym_error = dlerror();
      if (dlsym_error) {
         std::cout << "Cannot load symbol destroy: " << dlsym_error << '\n';
         throw;
      }
      destroy_triangle(nativeBehaviour.obj);
      nativeBehaviour.obj = nullptr;
   }
   

   Debug::log("Stopped Game");
}


} // namespace Ease
