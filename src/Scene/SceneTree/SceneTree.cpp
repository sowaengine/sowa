#include "SceneTree.hpp"

#include "Node.hpp"

namespace Ease
{

SceneTree::SceneTree()
{
   m_RootNode = createNode("Root");
}
   
} // namespace Ease
