#include "node.hpp"

#include "global.hpp"
#include <iostream>
#include <chrono>
#include "scene/components/components.hpp"
#include "util/utils.hpp"

Ease::Node* RootNode;
entt::registry sceneRegistry;

void destroyNode(Ease::Node* node);

namespace Ease
{
   Node::Node()
   : m_Name("Node"), m_UUID(90000)
   {
      m_UUID = Util::generateUUID();


      m_EntityID = sceneRegistry.create();
      m_sceneRegistry = &sceneRegistry;
   }

   Node::~Node()
   {
      std::cout << "Deleting: " << this->m_Name << std::endl;
   }

   void Node::addChildren(Node* node)
   {
      // first check if its already its child
      node->m_Parent = this;
      m_Children.push_back(node);
   }

   void Node::free()
   {
      delete this;
   }


   void Node::resetScene()
   {
      if(RootNode)
         for(auto* child : RootNode->getChildren())
         {
            std::cout << "Name: " << child->m_Name << std::endl;
            destroyNode(child);
         }
      
   }
}
void destroyNode(Ease::Node* node)
{
   if(node)
   {
      for(Ease::Node* _node : node->getChildren())
      {
         if(_node)
         {
            destroyNode(_node);
            if( sceneRegistry.valid(_node->getEntityID()) )
               sceneRegistry.destroy(_node->getEntityID());

         }
      }
      node->free();
   }
}