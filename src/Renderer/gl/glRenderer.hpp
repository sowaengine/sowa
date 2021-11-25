#pragma once
#include "glad/glad.h"

#include "Ease.hpp"
#include "Renderer/Model/Model.hpp"

#include "Application/Window/Window.hpp"
#include "Renderer/Shader/Shader.hpp"

#include "Scene/Components.hpp"

// call opengl functions with gl(func)
#define gl(x) glClearError();\
   x;\
   ASSERT(glLogCall(#x, __FILE__, __LINE__))


static void glClearError();
static bool glLogCall(const char* function, const char* file, int line);

void glClearError()
{
   while(glGetError() != GL_NO_ERROR);
}
bool glLogCall(const char* function, const char* file, int line)
{
   while(GLenum error = glGetError())
   {
      std::cout << "OpenGL ERROR: " << error <<  " Function: " << function << "File: " << file << ":" << line <<  std::endl;
      return false;
   }
   return true;
}

namespace Ease
{

class GLRenderer
{
public:
   static void drawModel(const Model& model);
   static void drawModels(bool useEditorCamera, const Transform3DComponent& cameraTransform, const CameraComponent& camera);
   /**
    * @param useScreenSize -> should node be drawen to target resolution or window resolution
    * true -> use target resolution (NOT SCREEN)
    * @note CHECK IMPLEMENTATION BEFORE USING IT. ITS PROBABLY NOT THE THING YOU WANT
   */
   static void drawMesh(Shader* shader, const Transform3DComponent& transform, const MeshComponent& mesh);

   static void setWindow(Window* _window);

// false means camera is on x:0 y:0
   static void setCameraTransformEnabled(bool value);
private:
   GLRenderer();
};
   
} // namespace Ease
