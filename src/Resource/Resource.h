#ifndef RESOURCE_H
#define RESOURCE_H

#pragma once

#include <stdint.h>

typedef uint32_t ResourceID;

namespace Ease
{
   
   class BaseResource
   {
      public:
         ResourceID GetResourceID() { return m_ResourceID; }
         void SetResourceID(ResourceID id) { m_ResourceID = id; }
         
         BaseResource() = default;
         ~BaseResource() = default;
      private:
         ResourceID m_ResourceID;
   };

} // namespace Ease


#endif