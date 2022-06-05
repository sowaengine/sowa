#include "NativeBehaviour.h"

namespace Ease
{
   NativeBehaviour::NativeBehaviour()
      : m_pBehaviour(nullptr)
   {

   }

   NativeBehaviour::~NativeBehaviour()
   {
      Delete();
   }
   
   void NativeBehaviour::Create()
   {
      if(m_pBehaviour != nullptr)
         Delete();
      m_pBehaviour = m_CreateFunc();
   }
   
   void NativeBehaviour::Delete()
   {
      if(m_pBehaviour != nullptr)
         m_DeleteFunc(m_pBehaviour);
      m_pBehaviour = nullptr;
   }
   
   void NativeBehaviour::CallStart()
   {
      m_pBehaviour->Start();
   }
   
   void NativeBehaviour::CallUpdate()
   {
      m_pBehaviour->Update();
   }
} // namespace Ease