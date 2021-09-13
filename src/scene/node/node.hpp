#ifndef __NODE_HPP__
#define __NODE_HPP__

/**
 * Node class is the only structure that makes the scene. There is no additional "Scene" class, root node is the scene root
 */

#include <vector>
#include <string>
#include "entt/entt.hpp"

namespace Ease
{
   class Node
   {
      private:
         Node* m_Parent;
         std::vector<Node*> m_Children;

         std::string m_Name;
         unsigned int m_UUID;

         entt::entity m_EntityID;


         entt::registry* m_sceneRegistry;
      public:
         Node();

         unsigned int getUUID() { return m_UUID; }
         std::string& getName() { return m_Name; }
         void setName(const char* name) { m_Name = name; }

         std::vector<Node*>& getChildren() { return m_Children; }

         void addChildren(Node* node);

         template<typename T>
         void addComponent()
         {
            m_sceneRegistry->emplace<T>(m_EntityID);
         }
         template<typename T>
         bool hasComponent()
         {
            auto view = m_sceneRegistry->view<T>();
            for(auto entity: view) {
               if(entity == m_EntityID)
                  return true;
            }
            return false;
         }

         /**
          * Destroys all nodes, initializes a new scene registry
          **/
         static void resetScene();
   };
}

#endif // __NODE_HPP__