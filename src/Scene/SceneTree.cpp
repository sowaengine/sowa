#include "SceneTree.hpp"
#include "Global.hpp"
#include "Node.hpp"

#include "entt/entt.hpp"



namespace Ease
{

//static
void cl_SceneTree::Init()
{
   static cl_SceneTree tree{};
   Global::SceneTree = &tree;

   if(Global::SceneTree->m_RootNode == nullptr)
   {
      Node* node = new Node();
      node->m_Name = "Root";
      Global::SceneTree->m_RootNode = node;
   }
}


Node* cl_SceneTree::NewNode(const std::string& name /*= "New Node"*/)
{
   Node* node = new Node();
   entt::entity entity = m_Registry.create();
   node->m_NodeID = (uint32_t)entity;
   node->m_Name = name;
   node->m_pRegistry = &m_Registry;

   Global::SceneTree->GetRoot()->AddChild(node);
   
   m_Nodes[node->m_NodeID] = node;
   return node;
}
   
} // namespace Ease
