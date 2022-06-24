#ifndef _E_EASE_HPP__
#define _E_EASE_HPP__
#pragma once

#include <string>
#include <unordered_map>
#include "ECS/Entity/Entity.hpp"


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
   }; Factory* factory = new Factory; \
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

         Ease::Entity self{};
   };
   struct NativeBehaviourFactory
   {
      public:
         virtual NativeBehaviour* Create() { return nullptr; }
         virtual void Destroy(NativeBehaviour*) {}
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

         std::unordered_map<std::string, NativeBehaviourFactory*> nativeBehaviours;
   };
}


#endif