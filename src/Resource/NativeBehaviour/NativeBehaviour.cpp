#include "NativeBehaviour.h"

namespace Ease
{
   NativeBehaviour::NativeBehaviour()
   {

   }

   NativeBehaviour::~NativeBehaviour()
   {
      Delete();
   }
   
   Ease::BaseBehaviour* NativeBehaviour::Create()
   {
      Ease::BaseBehaviour* behaviour = m_CreateFunc();
      m_Behaviours.push_back(behaviour);
      return behaviour;
   }
   
   void NativeBehaviour::Delete()
   {
      for(size_t i=0; i<m_Behaviours.size(); i++)
      {
         m_DeleteFunc(m_Behaviours[i]);
      }
      m_Behaviours.clear();
   }
} // namespace Ease