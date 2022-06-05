#ifndef EASEENGINE_H
#define EASEENGINE_H
#pragma once

#include <string>
#include <unordered_map>
#include <ECS/Entity/Entity.h>


#define EASE_VERSION_MAJOR 0
#define EASE_VERSION_MINOR 1
#define EASE_VERSION_PATCH 0

#define EASE_VERSION_STRING "0.1.0"

#define MAX(a, b) ((a)>(b)? (a) : (b))
#define MIN(a, b) ((a)<(b)? (a) : (b))

typedef void(*UserFunc)();

namespace Ease
{
   class Entity;

   class BaseModule
   {
      public:
         BaseModule() {}
         virtual ~BaseModule() {}

         virtual void Start() {}
         virtual void Update() {}
         virtual void OnImGuiRender() {}

         struct {
            std::string authorName = "";
            std::string moduleName = "";
            uint32_t version;

         } metadata;


         struct UserValue
         {
            std::string str_value{};
         };
         std::unordered_map<std::string, UserFunc> userFuncs;
         std::unordered_map<std::string, UserValue> userValues;
   };

   class BaseBehaviour
   {
      public:
         BaseBehaviour() {}
         virtual ~BaseBehaviour() {}

         virtual void Start() {}
         virtual void Update() {}

         Ease::Entity self;
   };
}


#endif