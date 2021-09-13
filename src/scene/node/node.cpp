#include "node.hpp"

#include "global.hpp"
#include <iostream>
#include <chrono>
#include "scene/components/components.hpp"

Ease::Node* RootNode;
entt::registry sceneRegistry;

namespace Ease
{
   Node::Node()
   : m_Name("Node"), m_UUID(90000)
   {
      /**
       * this randomizing is a temporary solution
       * !Come back here later
       */
      for(int i=0; i<time(NULL)%22; i++) {
         rand();
      }
      m_UUID = rand();

      m_EntityID = sceneRegistry.create();
      m_sceneRegistry = &sceneRegistry;
      addComponent<Comp::Entity>();
   }


   void Node::addChildren(Node* node)
   {
      // first check if its already its child
      node->m_Parent = this;
      m_Children.push_back(node);
   }


   void Node::resetScene()
   {
      // TODO: delete all nodes
   }
}