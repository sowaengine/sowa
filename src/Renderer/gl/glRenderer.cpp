#include "glRenderer.hpp"

#include "Ease.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "entt/entt.hpp"

#include "Global.hpp"
#include "Scene/Components.hpp"

#include "Application/Window/Window.hpp"


namespace Ease
{

cl_Stats* stats;

static Window* window;
void GLRenderer::setWindow(Window* _window)
{
   window = _window;
}

static bool cameraTransformEnabled = true;


GLRenderer::GLRenderer()
{
   
}

// static
void GLRenderer::drawModel(const Model& model)
{
   // model
   glm::mat4 transform{1.0f};
   transform = glm::translate(transform, {1280/2, 720/2, 0});
   transform = glm::rotate(transform, glm::radians(25.f), {0.f, 0.f, 1.f});
   transform = glm::scale(transform, {256, 256, 1});

   // view
   glm::mat4 view{1.0f};
   view = glm::translate(view, {0.f, 5.f, 0.0f});

   // projection
   glm::mat4 proj = glm::ortho(0.f, 1280.f, 0.f, 720.f, -1.f, 1.f);


   model.m_Shader->bind();
   model.m_Shader->bindUniform("u_Model", transform);
   model.m_Shader->bindUniform("u_View", view);
   model.m_Shader->bindUniform("u_Proj", proj);
   
   glBindBuffer(GL_ARRAY_BUFFER, model.m_VertexBuffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.m_IndexBuffer);
   glDrawElements(GL_TRIANGLES, model.m_Indices.size(), GL_UNSIGNED_INT, nullptr);


   
}
float cameraX = 0;
int x=0;
void GLRenderer::drawModels(bool useEditorCamera, const Transform3DComponent& cameraTransform, const CameraComponent& camera)
{
   using namespace Global;
   using namespace Comp;

   int width=1280, height=720;
   window->getVideoSize(&width, &height);
   glViewport(0, 0, width, height);


   // CameraComponent
   glm::mat4 view{1.0f};
   glm::mat4 proj;

   bool found = false;
   if(!useEditorCamera)
   {
      // find current camera and apply
      for(auto entity: Global::registry.view<Transform3DComponent, CameraComponent>()) {
         auto& transformComp = Global::registry.get<Transform3DComponent>(entity);
         auto& cameraComp = Global::registry.get<CameraComponent>(entity);

         if(cameraComp.current)
         {
            proj = cameraComp.getProjection();
            view = transformComp.getTransform();

            found = true;
            break;
         }
      }
   }
   if(useEditorCamera || !found)
   {
      static int i =0;
      //std::cout << "Fallback " << i++ << std::endl; 
      //view = glm::translate(view, {-width*0.5f, -height*0.5f, 0.f});
      //view = glm::scale(view, cameraTransform.scale);
      //view = glm::translate(view, cameraTransform.translation);
      view = cameraTransform.getTransform();
      proj = camera.getProjection();
   }

   view = glm::inverse(view);
   


   

   

   for(auto entity: Global::registry.view<Transform3DComponent, MeshComponent>()) {
      auto& transform = Global::registry.get<Transform3DComponent>(entity);
      auto& mesh = Global::registry.get<MeshComponent>(entity);

      if(!mesh.Visible) continue;

      glm::mat4 modelMat{1.0f};
      modelMat = transform.getTransform();
      //modelMat = glm::translate(modelMat, transform.translation*glm::vec2(1.f, -1.f, 1.f));
      //modelMat = glm::rotate(modelMat, transform.rotationRad, {0.f, 0.f, 1.f});
      //modelMat = glm::rotate(modelMat, glm::radians( (float)(transform.position.x+x) ), {0.f, 0.f, 1.f});
      //modelMat = glm::scale(modelMat, {transform.scale, 1});


      mesh.model->m_Shader->bind();
      mesh.model->m_Shader->bindUniform("u_Model", modelMat);
      mesh.model->m_Shader->bindUniform("u_View", view);
      mesh.model->m_Shader->bindUniform("u_Proj", proj);

      if(mesh.material)
      {
         mesh.model->m_Shader->bindUniform("u_Color", mesh.material->color);
         
         // if there is albedo texture, bind it to slot 0
         if(mesh.material->albedoTexture) {
            mesh.material->albedoTexture->bind(0);
            mesh.model->m_Shader->bindUniform("u_AlbedoTexture", 0);
         }
      }


      //mesh.model->m_Shader->bindUniform("u_Texture", 0); // texture is bound to slot 0

      glBindBuffer(GL_ARRAY_BUFFER, mesh.model->m_VertexBuffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.model->m_IndexBuffer);
      glDrawElements(GL_TRIANGLES, mesh.model->m_Indices.size(), GL_UNSIGNED_INT, nullptr);
      stats->drawCalls++;   
   }
   
}


void GLRenderer::drawMesh(Shader* shader, const Transform3DComponent& transform, const MeshComponent& mesh)
{
   using namespace Global;
   using namespace Comp;

   // CameraComponent view matrix
   glm::mat4 view{1.0f};
   int width=1280, height=720;
   window->getWindowSize(&width, &height);
   glViewport(0, 0, width, height);
   
   glm::mat4 proj = glm::ortho(0.f, (float)width, 0.f, (float)height, -1.f, 1.f);

   if(!mesh.Visible) return;

   glm::mat4 modelMat{1.0f};
   modelMat = glm::translate(modelMat, transform.translation);
   modelMat = glm::scale(modelMat, transform.scale);
   //modelMat = glm::scale(modelMat, {width, height, 1});

   shader->bind();
   shader->bindUniform("u_Model", modelMat);
   shader->bindUniform("u_View", view);
   shader->bindUniform("u_Proj", proj);

   if(mesh.material)
   {
      shader->bindUniform("u_Color", {1.f, 1.f, 1.f, 1.f});
      
      // if there is albedo texture, bind it to slot 0
      if(mesh.material->albedoTexture) {
         mesh.material->albedoTexture->bind(0);
         shader->bindUniform("u_AlbedoTexture", 0);
      }
   }

   glBindBuffer(GL_ARRAY_BUFFER, mesh.model->m_VertexBuffer);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.model->m_IndexBuffer);
   glDrawElements(GL_TRIANGLES, mesh.model->m_Indices.size(), GL_UNSIGNED_INT, nullptr);
   stats->drawCalls++;
}


void GLRenderer::setCameraTransformEnabled(bool value) {
   cameraTransformEnabled = value;
}
   
} // namespace Ease
