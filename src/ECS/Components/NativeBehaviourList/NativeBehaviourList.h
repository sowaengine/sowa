#ifndef NATIVEBEHAVIOURLIST_H
#define NATIVEBEHAVIOURLIST_H

#pragma once
#include <vector>
#include "Resource/NativeBehaviour/NativeBehaviour.h"

namespace Ease::Component
{
   class NativeBehaviourList
   {
   public:
      NativeBehaviourList();
      ~NativeBehaviourList();
      
      inline std::vector<ResourceID>& GetList() { return m_List; }
      inline std::vector<Ease::BaseBehaviour*>& GetBehaviourList() { return m_BehaviourList; }
   private:
      std::vector<ResourceID> m_List;
      std::vector<Ease::BaseBehaviour*> m_BehaviourList;
   };
}

#endif