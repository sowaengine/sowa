#pragma once

#include <memory>
#include <vector>
#include <iostream>

#include "entt/entt.hpp"

namespace Ease
{

class Node
{
public:
   Node(const std::string& name = std::string());

   template<typename T, typename... Args>
   T& addComponent(Args&&... args)
   {
      return m_Registry->emplace<T>(m_EntityID, std::forward<Args>(args)...);
   }

   template<typename T>
   T& getComponent()
   {
      return m_Registry->get<T>(m_EntityID);
   }

   template<typename T>
   void removeComponent()
   {
      m_Registry->remove<T>(m_EntityID);
   }

   template<typename T>
   bool hasComponent()
   {
      auto comp = m_Registry->try_get<T>(m_EntityID);
      return comp != NULL;
   }

   bool operator==(const Node& other) const { return m_EntityID == other.m_EntityID; }
   bool operator==(const Node& other) { return m_EntityID == other.m_EntityID; }
   bool operator!=(const Node& other) { return m_EntityID != other.m_EntityID; }

   std::vector<Node*> getChildren() { return m_Children; }
   Node* getChild(const std::string& name);
   bool hasChild(Node* node) {
      for(auto* child : m_Children) {
         if(*child == *node) return true;
      }
      return false;
   }
// path can be child/child/child
   Node* getNode(const std::string& path);
   const std::string& getName() { return m_Name; }
   void setName(const std::string& name) { m_Name = name; }
   void removeChild(Node* node);
   bool hasParent() { return m_Parent != nullptr; }

// returns true if node has any lower relation with this
   bool hasChildRecursive(Node* node);

   void setParent(Node* node) { if(hasParent()){m_Parent->removeChild(this);}  m_Parent = node; }
   Node* getParent() { return m_Parent; }

   void addChild(Node* node);

// returns like "root/parent/node"
   std::string getPath();
// 
   void setPath(const std::string& path);

   //bool operator==(const Node& other) { return m_EntityID == other.m_EntityID; }
   std::string m_Name;

// deletes the node with all its children and grand children
   void free();
private:

   Node* m_Parent = nullptr;
   std::vector<Node*> m_Children;

   entt::entity m_EntityID;

   entt::registry* m_Registry;
};
   
} // namespace Ease
