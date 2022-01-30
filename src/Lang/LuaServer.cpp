#include "LuaServer.hpp"  
#include "Scene/Components.hpp"
#include "Scene/Entity.hpp"

#include "sol/sol.hpp"
#include "Debug.hpp"

#include "Scene/Scene.hpp"
#include "Application/Input.hpp"

LuaServer::LuaServer()
{
	m_State.open_libraries(sol::lib::base);

   m_State.new_usertype<glm::vec2>("Vector2",
      sol::constructors<void(), void(float, float)>(),
      "x", &glm::vec2::x,
      "y", &glm::vec2::y);
   m_State.new_usertype<glm::vec3>("Vector3",
      sol::constructors<void(), void(float, float, float)>(),
      "x", &glm::vec3::x,
      "y", &glm::vec3::y,
      "z", &glm::vec3::z);
   
   m_State.new_usertype<Transform2DComponent>("Transform2D",
      "position", &Transform2DComponent::position,
      "rotation", &Transform2DComponent::rotation,
      "scale", &Transform2DComponent::scale);
   

   m_State.new_usertype<Entity>("Entity",
      "name", sol::property(&Entity::GetName, &Entity::SetName),
      "GetTransform2D", &Entity::getComponent<Transform2DComponent>);

   m_State.new_enum("Key",
      "W", Input::Key::W,
      "A", Input::Key::A,
      "S", Input::Key::S,
      "D", Input::Key::D,
      "LEFT", Input::Key::LEFT,
      "RIGHT", Input::Key::RIGHT,
      "UP", Input::Key::UP,
      "DOWN", Input::Key::DOWN);
   
   m_State.create_named_table("Input",
      "IsKeyPressed", &Input::IsKeyPressed);


   
   m_State.set_function("GetEntityByName", &Scene::GetEntityByName);
}
	
LuaServer::~LuaServer()
{
	
}

void LuaServer::BindLuaScriptComponent(LuaScriptComponent& component, Entity& entity) 
{
   if(component.script == nullptr)
      return;
   
   auto res = m_State.script_file(component.script->GetFilepath());
   if(!res.valid())
   {
      LOG_ERROR("Error Executing Script at %s, %s", component.script->GetFilepath().c_str(), lua_tostring(m_State, -1));
      return;
   }

   auto stateClassFunc = m_State["_init"];
   if(!stateClassFunc.valid() || stateClassFunc.get_type() != sol::type::function)
   {
      LOG_ERROR("Error Executing Script at %s, Class Not Valid", component.script->GetFilepath().c_str());
      return;
   }


   auto stateClass = stateClassFunc();
   if(!stateClass.valid() || stateClass.get_type() != sol::type::table)
   {
      LOG_ERROR("Error Executing Script at %s, Class Not Valid", component.script->GetFilepath().c_str());
      return;
   }
   



   
   auto startFunc = ((sol::table)stateClass)["Start"];
   if(startFunc.valid() && startFunc.get_type() == sol::type::function)
   {
      m_State["this"] = entity;
      auto res = startFunc();
      if(!res.valid())
      {
         LOG_ERROR("Error Executing Script at %s, %s", component.script->GetFilepath().c_str(), lua_tostring(m_State, -1));
         return;
      }
   }

   auto updateFunc = ((sol::table)stateClass)["Update"];
   if(updateFunc.valid() && updateFunc.get_type() == sol::type::function)
   {
      component.updateFunc = (sol::function)updateFunc;
   }
}


LuaServer& LuaServer::get_singleton() 
{
   static LuaServer server;
   return server;
}
