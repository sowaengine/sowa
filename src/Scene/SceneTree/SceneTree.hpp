#pragma once
#include <iostream>
#include <memory>

#include "Node.hpp"
namespace Ease
{

class SceneTree
{
public:
   SceneTree();
   
   Node* getRoot() {return m_RootNode;}

   Node* createNode(const std::string& name = std::string())
   {
      //std::shared_ptr<Node> node = std::make_shared<Node>(name);
      Node* node = new Node(name);
      return node;
   }

private:
   Node* m_RootNode;
};
   
} // namespace Ease
