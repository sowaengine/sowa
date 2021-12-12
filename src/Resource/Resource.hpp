/**
 * @file Resource.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once

#include "ResourceManager/ResourceManager.hpp"

#include "Global.hpp"

namespace Ease
{

class Resource
{
public:
   ResourceID ID() { return m_ID; }

   Resource() = default;
   ~Resource()
   {
      Global::ResourceManager->DestroyResource(m_ID);
   }
private:
   ResourceID m_ID;

   // m_ID set by ResourceManager
   friend class cl_ResourceManager;

};
   
} // namespace Ease
