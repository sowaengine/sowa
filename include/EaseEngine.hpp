/*
   Necessary headers for Ease Engine
*/

#ifndef __EASE_ENGINE_HPP__
#define __EASE_ENGINE_HPP__

#include <iostream>
#include <memory>



namespace Ease
{
   class Node;
   struct Transform3DComponent;
   
}


class NativeScript
{
public:
   NativeScript() {}
   virtual ~NativeScript() {}

   virtual void Start() {};
   virtual void Update() {};
};

typedef NativeScript* create_t();
typedef void destroy_t(NativeScript*);


// Components


#endif