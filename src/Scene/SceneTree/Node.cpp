#include "Node.hpp"
#include "entt/entt.hpp"

#include "Scene/SceneTree/SceneTree.hpp"
#include "Global.hpp"


namespace Ease
{
entt::registry Global::registry;

Node::Node(const std::string& name)
{
   m_EntityID = Global::registry.create();
   
   m_Name = std::string();
   m_Name = name.empty() ? "New Node" : name;

   m_Parent = nullptr;

   m_Registry = &Global::registry;
}


void Node::addChild(Node* node)
{
   if(node->getParent() != nullptr)
   {
      std::cout << "ERROR: " << "Node(" << node->m_Name << ") Already has a parent." << std::endl;
      return;
   }

   m_Children.push_back(node);
   node->setParent(this);
}

static void getPathRecursive(Node* node, std::string& pathStr)
{
   if(node == nullptr) return;
   Node* parent = node->getParent();
   if(parent != nullptr)
   {
      getPathRecursive(parent, pathStr);
      pathStr += parent->getName() + "/";
   }

}
std::string Node::getPath()
{
   std::string pathStr = "";
   getPathRecursive(this, pathStr);
   return pathStr + getName();
}

Node* Node::getChild(const std::string& name)
{
   for(auto* child : m_Children)
   {
      if(child->getName() == name)
      {
         return child;
      }
   }
   return nullptr;
}

void Node::setPath(const std::string& path)
{
   std::vector<std::string> nodes;
   std::string _path = path + "/";

   size_t pos = 0;
   std::string token;
   while ((pos = _path.find("/")) != std::string::npos) {
      token = _path.substr(0, pos);
      nodes.push_back(token);
      _path.erase(0, pos + 1);
   }
   
   if(nodes.size() <= 1)
   {
      std::cout << "Couldn't set path of: " << getName() << std::endl;
      return;
   }
   
   Node* nodeToAddSelf = Global::sceneTree.getRoot();
   size_t iter = 1; // 0 is scene root
   while(true)
   {
      Node* nextNode = nullptr;
      nextNode = nodeToAddSelf->getChild(nodes[iter]);
      if(nextNode == nullptr) break;
      if(iter == nodes.size()) break;

      nodeToAddSelf = nextNode;
      iter++;
   }

   setParent(nullptr);
   nodeToAddSelf->addChild(this);
}


void Node::removeChild(Node* node)
{
   for(size_t i=0; i<m_Children.size(); i++)
   {
      if(m_Children[i]->m_EntityID == node->m_EntityID)
      {
         m_Children.erase(m_Children.begin() + i);
         return;
      }
   }
   std::cout << "Node: " << getName() << " doesn't have child: " << node->getName() << ", so can't remove" << std::endl;
}

Node* Node::getNode(const std::string& path)
{
   std::vector<std::string> nodes;
   std::string _path = path + "/";

   size_t pos = 0;
   std::string token;
   while ((pos = _path.find("/")) != std::string::npos) {
      token = _path.substr(0, pos);
      nodes.push_back(token);
      _path.erase(0, pos + 1);
   }
   if(nodes.size() <= 1)
   {
      std::cout << "Couldn't get path of: " << getName() << std::endl;
      return this;
   }

   Node* nodeToAddSelf = this;
   size_t iter = 1; // 0 is scene root
   while(true)
   {
      Node* nextNode = nullptr;
      nextNode = nodeToAddSelf->getChild(nodes[iter]);
      if(nextNode == nullptr) break;
      if(iter == nodes.size()) break;

      nodeToAddSelf = nextNode;
      iter++;
   }
   
   return nodeToAddSelf;
}


bool Node::hasChildRecursive(Node* node)
{
   if(hasChild(node)) return true;
   
   for(auto* child : m_Children) {
      if(child->hasChildRecursive(node)) return true;
   }
   return false;
}

void Node::free()
{
   // delete all children
   // calling free on children will free their children recursively
   for(auto* child : m_Children)
   {
      child->free();
   }
   
   Debug::log("Deleting node: " + getPath());
   setParent(nullptr);
   delete this;
}

} // namespace Ease
