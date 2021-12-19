/**
 * @file SceneTree.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once

#include "Node.hpp"

#include <vector>
#include <unordered_map>

#include "entt/entt.hpp"


namespace Ease
{

class cl_SceneTree
{
   public:
      static void Init();

      Node* GetRoot() { return m_RootNode; }

      Node* NewNode(const std::string& name = "New Node");

      entt::registry* GetRegistry() { return &m_Registry; }
   private:
      Node* m_RootNode = nullptr;

      // Keeps track of all nodes in scene.
      // std::vector<Node*> m_Nodes;

      

      std::unordered_map<NodeID, Node*> m_Nodes;

      entt::registry m_Registry;
};
   
} // namespace Ease
