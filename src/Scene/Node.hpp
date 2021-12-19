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

#include "entt/entt.hpp"


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


      template<typename T, typename... Args>
      T& addComponent(Args&&... args)
      {
         return m_pRegistry->emplace<T>((entt::entity)m_NodeID, std::forward<Args>(args)...);
      }

      template<typename T>
      T& getComponent()
      {
         return m_pRegistry->get<T>((entt::entity)m_NodeID);
      }

      template<typename T>
      void removeComponent()
      {
         m_pRegistry->remove<T>((entt::entity)m_NodeID);
      }

      template<typename T>
      bool hasComponent()
      {
         auto comp = m_pRegistry->try_get<T>((entt::entity)m_NodeID);
         return comp != NULL;
      }
      

      bool operator==(const Node& other) const { return m_NodeID == other.m_NodeID; }
      bool operator==(const Node& other) { return m_NodeID == other.m_NodeID; }
      bool operator!=(const Node& other) { return m_NodeID != other.m_NodeID; }

   private:
      friend class cl_SceneTree;
      NodeID m_NodeID = 0;

      std::string m_Name = "Node";

      Node* m_Parent;
      std::vector<Node*> m_Children;


      entt::registry* m_pRegistry;
};

} // namespace Ease
