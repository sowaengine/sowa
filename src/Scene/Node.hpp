/**
 * @file Node.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2021-12-12
 */
#pragma once

#include <vector>
#include "Debug.hpp"

typedef uint32_t NodeID;

namespace Ease
{

class Node
{
   public:
      void AddChild(Node* node);
      bool HasChild(Node* node);
      bool HasChild(const std::string& name);

      void SetParent(Node* parent);

      // Returns Node path like "parent/parent/node/child"
      std::string GetPath();
      std::string GetName() { return m_Name; }
      Node* GetNode(std::string path);
      Node* GetChild(const std::string& name);


      Node* GetParent() { return m_Parent; }
      const std::vector<Node*>& GetChildren() { return m_Children; }

      

      bool operator==(const Node& other) const { return m_NodeID == other.m_NodeID; }
      bool operator==(const Node& other) { return m_NodeID == other.m_NodeID; }
      bool operator!=(const Node& other) { return m_NodeID != other.m_NodeID; }

   private:
      friend class cl_SceneTree;
      NodeID m_NodeID = 0;

      std::string m_Name = "Node";

      Node* m_Parent;
      std::vector<Node*> m_Children;
};

} // namespace Ease
