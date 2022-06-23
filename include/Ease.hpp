#ifndef EASEENGINE_H
#define EASEENGINE_H
#pragma once

#include <string>
#include <unordered_map>
#include "ECS/Entity/Entity.h"


#define EASE_VERSION_MAJOR 0
#define EASE_VERSION_MINOR 1
#define EASE_VERSION_PATCH 0

#define EASE_VERSION_STRING "0.1.0"

#define MAX(a, b) ((a)>(b)? (a) : (b))
#define MIN(a, b) ((a)<(b)? (a) : (b))

#define EASEMODULE_BIND_NATIVEBEHAVIOUR(lib, Behaviour) do { \
   struct Factory : Ease::NativeBehaviourFactory \
   { \
      public: \
         Ease::NativeBehaviour* Create() override \
         { \
            Behaviour* b = new TestComponent(); \
            return b; \
         } \
         void Destroy(Ease::NativeBehaviour* b) override \
         { \
            delete reinterpret_cast<Behaviour*>(b); \
         } \
          \
   } factory; \
   lib->nativeBehaviours[#Behaviour] = factory; \
} while(0); \

typedef void(*UserFunc)();

namespace Ease
{
   class Entity;


   class NativeBehaviour
   {
      public:
         virtual void Start() {}
         virtual void Update() {}
   };
   struct NativeBehaviourFactory
   {
      public:
         virtual NativeBehaviour* Create() = 0;
         virtual void Destroy(NativeBehaviour*) = 0;
   };


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

         std::unordered_map<std::string, NativeBehaviourFactory> nativeBehaviours;
   };
}


#endif