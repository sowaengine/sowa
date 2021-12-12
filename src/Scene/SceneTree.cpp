#include "SceneTree.hpp"
#include "Global.hpp"
#include "Node.hpp"


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
   static int i = 12;
   Node* node = new Node();
   node->m_NodeID = i++; // todo entt::registry.create()
   node->m_Name = name;

   Global::SceneTree->GetRoot()->AddChild(node);
   
   m_Nodes[node->m_NodeID] = node;
   return node;
}
   
} // namespace Ease
