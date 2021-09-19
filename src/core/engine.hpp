#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__



namespace Ease
{
   class Engine
   {
      private:
         bool m_Running;

      public:
         Engine();
         ~Engine() {}

         // return 1 -> Success
         // anything other than 1 -> Error
         unsigned int Update();

         void SetRunning(bool will_run);
         bool isRunning() { return m_Running; }
   };
}

#endif // __ENGINE_HPP__