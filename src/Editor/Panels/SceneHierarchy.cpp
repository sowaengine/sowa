#include "Ease.hpp"
#include "Editor/Editor.hpp"


#include "imgui/imgui.h"
#include "Utils/File.hpp"

#include "Global.hpp"

#include "Scene/SceneTree/SceneTree.hpp"


namespace Ease
{

// node that is currently selected by right clicking
// used for popup: "scene_rclick"
static Node* rightClickedNode = nullptr;

static void drawNodeRecursive(Node& node, EditorData& editorData, ImGuiTreeNodeFlags flags = 0)
{
   bool singleClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
   // bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
   bool rightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

   // double click -> open children
   // single click -> show properties
   bool opened = ImGui::TreeNodeEx(node.getName().c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen | flags);
   //bool opened = ImGui::CollapsingHeader(node.getName().c_str(),  ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen | flags);
   
   // Region node right click menu
   // set to scene root if there is no node selected
   if(ImGui::IsItemHovered() && rightClicked)
   {
      rightClickedNode = &node;
   }
   

   // Nodes are both drag source and target
   if(ImGui::BeginDragDropSource(0))
   {
      ImGui::SetDragDropPayload("node_item", node.getPath().c_str(), node.getPath().size(), ImGuiCond_Once);
      
      ImGui::EndDragDropSource();
   }
   if(ImGui::BeginDragDropTarget())
   {
      const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("node_item", 0);
      if(payload != nullptr)
      {
         const char* path = (const char*)payload->Data;
         Node* nodeSrc = Global::sceneTree.getRoot()->getNode(path);
         // if not trying to drag scene root
         if(*nodeSrc != *Global::sceneTree.getRoot())
         {
            // we don't want to add child if source node is already parent or grandparent of the destination node
            if(!nodeSrc->hasChildRecursive(&node))
            {
               nodeSrc->setParent(nullptr);
               node.addChild(nodeSrc);
            }
         }
      }

      ImGui::EndDragDropTarget();
   }

   if(ImGui::IsItemHovered())
   {
      if(singleClicked)
      {
         editorData.selectedNode = &node;
      }
   }
   if(opened)
   {
      for(auto child : node.getChildren())
      {
         ImGuiTreeNodeFlags childflags = 0;

         if(child->getChildren().size() == 0)
            childflags |= ImGuiTreeNodeFlags_Bullet;
         
         //ImGui::Indent();
         drawNodeRecursive(*child, editorData, childflags);
         //ImGui::Unindent();
      }
   }
   if(opened)
      ImGui::TreePop();
   
}
// unused
static void printNodeRec(Node& node)
{
   return;
   //if(node == nullptr) return;

   static int indent = 0;
   
   for(int i=0; i<indent; i++)
   {
      std::cout << "\t";
   }
   std::cout << node.m_Name << std::endl;
   indent++;
   for(auto* child : node.getChildren())
   {
      printNodeRec(*child);
   }

}



void Editor::drawSceneHierarchy()
{
   if(ImGui::Begin("Scene"))
   {
      Node* rootNode = Global::sceneTree.getRoot();

      if(rootNode != nullptr)
         drawNodeRecursive( *rootNode, editorData, 0);//ImGuiTreeNodeFlags_DefaultOpen );

      
      // scene hierarchy right click popup menu
      bool rightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
      if(rightClicked && ImGui::IsWindowHovered())
      {
         ImGui::OpenPopup("scene_rclick");
      }
      if(ImGui::BeginPopup("scene_rclick"))
      {
         if(ImGui::MenuItem("New Node"))
         {
            if(rightClickedNode == nullptr)
               rightClickedNode = rootNode;

            rightClickedNode->addChild(new Node());
            rightClickedNode = nullptr;
         }

         if(ImGui::MenuItem("Delete Node"))
         {
            if(rightClickedNode == nullptr)
               rightClickedNode = rootNode;
            
            // if we are not trying to delete root node
            if(*rightClickedNode != *rootNode)
            {
               // checking editorData.selectedNode is not a nullptr first so if it's false it immediately exits and doesn't check if it's equal to rightClickedNode
               if( editorData.selectedNode != nullptr && *editorData.selectedNode == *rightClickedNode )
                  editorData.selectedNode = nullptr;


               rightClickedNode->free();
               rightClickedNode = nullptr;

            } else
            {
               printLine("Cannot delete root node of the scene!");
            }
         }


         ImGui::EndPopup();
      }


      static bool made = false;
      if(!made)
      {
         printNodeRec(*rootNode);
         made = true;
      }
   }
   ImGui::End();
}
   
} // namespace Ease
