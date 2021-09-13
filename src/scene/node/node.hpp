#ifndef __NODE_HPP__
#define __NODE_HPP__

/**
 * Node class is the only structure that makes the scene. There is no additional "Scene" class, root node is the scene root
 */

#include <vector>
#include <string>

namespace Ease
{
   class Node
   {
      private:
         Node* m_Parent;
         std::vector<Node*> m_Children;

         std::string m_Name;
         unsigned int m_UUID;
      public:
         Node();

         unsigned int getUUID() { return m_UUID; }
         std::string& getName() { return m_Name; }
         void setName(const char* name) { m_Name = name; }

         std::vector<Node*>& getChildren() { return m_Children; }

         void addChildren(Node* node);
   };
}

#endif // __NODE_HPP__