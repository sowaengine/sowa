#include "node.hpp"

#include "global.hpp"
#include <iostream>
#include <chrono>

Ease::Node* RootNode;

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
   }


   void Node::addChildren(Node* node)
   {
      // first check if its already its child
      node->m_Parent = this;
      m_Children.push_back(node);
   }
}