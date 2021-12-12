#include "Node.hpp"

#include <algorithm> // reverse

#include "SceneTree.hpp"
#include "Global.hpp"

namespace Ease
{

void Node::AddChild(Node* node)
{
   assert(node != nullptr);
   if(!HasChild(node))
   {
      if(node->GetParent() != nullptr)
      {
         if(*(node->GetParent()) != *this)
            node->SetParent(this);
      }
      else
      {
         node->SetParent(this);
      }

      m_Children.push_back(node);
   }
}


bool Node::HasChild(Node* node)
{
   assert(node != nullptr);
   for(auto child : m_Children)
   {
      if(child->m_NodeID == node->m_NodeID)
         return true;
   }
   return false;
}
bool Node::HasChild(const std::string& name)
{
   for(auto child : m_Children)
   {
      if(child->m_Name == name)
         return true;
   }
   return false;
}


Node* Node::GetChild(const std::string& name)
{
   for(auto child : m_Children)
   {
      if(child->m_Name == name)
         return child;
   }
   return nullptr;
}
   

void Node::SetParent(Node* parent)
{
   assert(parent != nullptr);
   //if(parent == nullptr || !parent->HasChild(this))
   {
      m_Parent = parent;
      //parent->AddChild(this);
   }
}


std::string Node::GetPath()
{
   Node* node = this;
   std::string BuildedString = "";

   do
   {
      std::string Name = node->m_Name;
      std::reverse(Name.begin(), Name.end());   
      BuildedString += Name + "/";

      node = node->GetParent();

      if(node == nullptr)
         break;
   } while (*node != *(Global::SceneTree->GetRoot()));
   

   std::reverse(BuildedString.begin(), BuildedString.end());
   return BuildedString;
}

Node* Node::GetNode(std::string path)
{
   LOG("%s", "Hey");
   std::vector<std::string> Children;


   size_t pos = 0;
   std::string token;
   while ((pos = path.find("/")) != std::string::npos) {
      token = path.substr(0, pos);
      if(token != "")
         Children.push_back(token);
      path.erase(0, pos+1);
   }
   Children.push_back(path);

   
   
   Node* node = this;
   for(auto childName : Children)
   {
      if(node->HasChild(childName))
      {
         node = node->GetChild(childName);
      }
      else
      {
         LOG_ERROR("Failed to find child node '%s'", childName.c_str());
         return nullptr;
      }
   }
   return node;
}

} // namespace Ease
