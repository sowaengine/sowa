#pragma once

#include "Transform2D/Transform2D.hpp"
#include "Mesh/Mesh.hpp"

#include "Lang/Lua/luaScript.hpp"


#include "EaseEngine.hpp"
#include "Lang/Native/NativeScriptResource.hpp"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Ease {

struct MeshComponent
{
   std::shared_ptr<Model> model;
   std::shared_ptr<Material> material;

   bool Visible = true;
};
struct Transform2DComponent
{
   Transform2DComponent() = default;
   Transform2DComponent(glm::vec2 position) { this->position = position; }
   
   glm::vec2 position{0.f, 0.f};
   float zIndex = 0.f;
   float rotationRad = 0.f;
   glm::vec2 scale{128.f, 128.f};

   glm::mat4 getTransform(bool inverseRotation = false)
   {
      glm::mat4 transform = glm::mat4{1.f};
      transform = glm::translate(transform, {position, zIndex});
      if(inverseRotation)
         transform = glm::rotate(transform, -rotationRad, {0.f, 0.f, 1.f});
      else
         transform = glm::rotate(transform, rotationRad, {0.f, 0.f, 1.f});
      transform = glm::scale(transform, glm::vec3(scale, 1.f));
      
      return transform;
   }
};
struct Transform3DComponent
{
   Transform3DComponent() = default;
   Transform3DComponent(glm::vec3 translation) { this->translation = translation; }
   
   glm::vec3 translation{0.f, 0.f, 0.f};
   glm::vec3 rotation{0.f, 0.f, 0.f};
   glm::vec3 scale{128.f, 128.f, 128.f};


// definetly need some refactor
   glm::mat4 getTransform(bool inverseRotation = false, bool yDown = true, bool inversedY = false, bool inversedX = false, bool inversedZ = false) const
   {
      glm::mat4 transform = glm::mat4{1.f};
      if(yDown)
         transform = glm::translate(transform, translation);
      else
         transform = glm::translate(transform, {translation.x, -translation.y, translation.z});


      if(inverseRotation)
      {
         transform *= glm::rotate(glm::mat4(1.f), rotation.y, {0.f, 1.f, 0.f});
         transform *= glm::rotate(glm::mat4(1.f), -rotation.x, {1.f, 0.f, 0.f});
         transform *= glm::rotate(glm::mat4(1.f), rotation.z, {0.f, 0.f, 1.f});
      }
      else
      {
         if(inversedY)
            transform *= glm::rotate(glm::mat4(1.f), rotation.y, {0.f, 1.f, 0.f});
         else
            transform *= glm::rotate(glm::mat4(1.f), rotation.y, {0.f, 1.f, 0.f});

         if(inversedX)
            transform *= glm::rotate(glm::mat4(1.f), -rotation.x, {1.f, 0.f, 0.f});
         else
            transform *= glm::rotate(glm::mat4(1.f), rotation.x, {1.f, 0.f, 0.f});

         if(inversedZ)
            transform *= glm::rotate(glm::mat4(1.f), -rotation.z, {0.f, 0.f, 1.f});
         else
            transform *= glm::rotate(glm::mat4(1.f), rotation.z, {0.f, 0.f, 1.f});
      }
      

      transform *= glm::scale(glm::mat4(1.f), scale);

      
      return transform;
   }
   glm::mat4 getTransformYDown(bool inverseRotation = false) const
   {
      glm::mat4 transform = glm::mat4{1.f};
      transform = glm::translate(transform, translation);
      
      transform = glm::rotate(transform, rotation.y, {0.f, 1.f, 0.f});
      transform = glm::rotate(transform, rotation.x, {1.f, 0.f, 0.f});
      transform = glm::rotate(transform, rotation.z, {0.f, 0.f, 1.f});
      
      transform = glm::scale(transform, scale);

      
      return transform;
   }
};

enum class CameraProjection
{
   ORTHOGRAPHIC = 0,
   PERSPECTIVE,
};
struct CameraComponent
{
   CameraComponent() {}

   CameraProjection projectionType = CameraProjection::ORTHOGRAPHIC;

// Common
   bool current = false;
   
// Ortographic
   glm::vec2 size{1280.f, 720.f};

// Perspective
   float fov = glm::radians(45.f);
   float aspect = 16.f/9.f;
   float near = 0.1f;
   float far = 100.f;


   glm::mat4 getProjection() const
   {
      if(projectionType == CameraProjection::PERSPECTIVE)
         return glm::perspective(fov, aspect, near, far);
      else
         return glm::ortho(0.f, (float)size.x, 0.f, (float)size.y, -1.f, 1.f);
   }
   // return CameraComponent Projection matrix with right handed system
   glm::mat4 getProjectionRH() const
   {
      assert(false);

      if(projectionType == CameraProjection::PERSPECTIVE)
         return glm::perspectiveRH(fov, aspect, near, far);
      else
         return glm::ortho(0.f, (float)size.x, 0.f, (float)size.y, -1.f, 1.f);
   }
};

// LuaBehaviour component is the component that will be attached to nodes;
// LuaScript is a resource object that will hold a lua script (.lua)
struct LuaBehaviour
{
   LuaScript script;
   std::string className = "";

   int scriptRef = -1;

   int startRef = -1;
   int updateRef = -1;
};


struct NativeBehaviour
{
   std::string sharedObjectPath = "";

   std::shared_ptr<NativeScriptResource> resource;
   NativeScript* obj;
};


} // namespace Ease
