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

#define WORLD_TO_SCREEN(x) (x * 25.f)
#define SCREEN_TO_WORLD(x) (x / 25.f)

#ifndef PI
    #define PI 3.1416f
#endif
#ifndef DEG2RAD
    #define DEG2RAD (PI/180.0f)
#endif
#ifndef RAD2DEG
    #define RAD2DEG (180.0f/PI)
#endif

#define EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_NORMAL   (1)
#define EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_HOVER    (2)
#define EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_CLICKED  (3)
#define EASE_DEFAULT_RESOURCE__TEXTURE2D_BUTTON_DISABLED (4)

#define EASE_BUILTIN_GLOBAL_RESOURCE_ID_MAX (1000)
#define EASE_USER_GLOBAL_RESOURCE_ID_MAX (10000)

#define EASEMODULE_BIND_NATIVEBEHAVIOUR(lib, Behaviour) do { \
   struct Factory : Ease::NativeBehaviourFactory \
   { \
      public: \
         Ease::NativeBehaviour* Create() override \
         { \
            Behaviour* b = new Behaviour(); \
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