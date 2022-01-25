/**
 * @file Components.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-26
 */
#ifndef COMPONENTS_H
#define COMPONENTS_H
#pragma once

#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <EaseGL.hpp>

// Holds data like name, id etc. all entities should have it
struct CommonComponent
{
   std::string name;


   CommonComponent(const std::string& _name)
      : name(_name)
   {
   }

};

struct Transform2DComponent
{
   glm::vec2 position;
   glm::vec2 scale;
   float rotation; // radians
   int zIndex = 0; // can be from 0 to 1000

   glm::mat4 GetTransform(glm::vec2 scaleMult = glm::vec2(1.0f, 1.0f))
   {
      glm::mat4 transform(1.0f);
      transform = glm::translate(transform, glm::vec3(position.x, position.y, 0.1f - (float)zIndex / 100.f));
      transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
      transform = glm::scale(transform, glm::vec3(scaleMult.x, scaleMult.y, 1.0));
      transform = glm::scale(transform, glm::vec3(scale.x, scale.y, 1.0));

      return transform;
   }

   Transform2DComponent() : position({0.f, 0.f}), scale({1.f, 1.f}), rotation(0.f) {}
   Transform2DComponent(const glm::vec2& _position, const glm::vec2& _scale, float _rotation)
      : position(_position), scale(_scale), rotation(_rotation)
   {
   }
};

struct SpriteComponent
{
   EaseGL::GLTexture* ptexture;

   SpriteComponent() : ptexture(nullptr) { }
   SpriteComponent(EaseGL::GLTexture* _ptexture)
      : ptexture(_ptexture)
   {
   }
};


struct CameraComponent
{
   bool current = false;
};


#endif