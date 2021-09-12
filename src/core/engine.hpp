#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__



namespace Ease
{
   class Engine
   {
      private:

      public:
         Engine() {}
         ~Engine() {}

         // return 1 -> Success
         // anything other than 1 -> Error
         unsigned int Update();
   };
}

#endif // __ENGINE_HPP__