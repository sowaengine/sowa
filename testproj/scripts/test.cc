#include "EaseEngine.hpp"

// this is a native object
// and bound to a node with NativeScriptComponent attached to it

class Test : public NativeScript
{
   void Start()
   {
      std::cout << "NativeScript Start" << std::endl;
   }

   void Update()
   {
      static int i = 0;
      i++;
      if(i > 60)
      {
         std::cout << "NativeScript Update" << std::endl;
         i = 0;
      }
   }
   

   ~Test()
   {
      std::cout << "Deleted" << std::endl;
   }
};


extern "C" NativeScript* create() {
   return new Test;
}
extern "C" void destroy(NativeScript* p) {
   delete p;
}