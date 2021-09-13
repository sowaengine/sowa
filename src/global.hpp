#ifndef __GLOBAL_HPP__
#define __GLOBAL_HPP__

#include "renderer/window/window.hpp"
#include "core/engine.hpp"
#include "scene/node/node.hpp"

#include "entt/entt.hpp"

/**
 * Global unit containing extern variables
 * that can be accessed from any other place
 */
/*namespace Ease
{
   namespace Global
   {*/

      extern Ease::Window* windowRef;
      extern Ease::Engine* EngineRef;

      extern Ease::Node* RootNode;
      extern entt::registry sceneRegistry;
   /*}
}*/




#endif // __GLOBAL_HPP__