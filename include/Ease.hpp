#ifndef EASEENGINE_H
#define EASEENGINE_H
#pragma once

#include <string>


#define EASE_VERSION_MAJOR 0
#define EASE_VERSION_MINOR 1
#define EASE_VERSION_PATCH 0

#define MAX(a, b) ((a)>(b)? (a) : (b))
#define MIN(a, b) ((a)<(b)? (a) : (b))


namespace Ease
{
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
   };

}


#endif