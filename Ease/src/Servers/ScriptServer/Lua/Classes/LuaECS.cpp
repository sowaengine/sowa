#include "Servers/ScriptServer/LuaScriptServer.hpp"

#include "ECS/Components/Components.hpp"

namespace Ease {

class ComponentBaseHandle {
  protected:
	Ease::Entity _handle;
};

template <typename T>
class ComponentHandle : public T, public ComponentBaseHandle {
  public:
	template <typename Val, auto Member>
	Val get_item() {
		T &comp = _handle.GetComponent<T>();
		Val &val = comp.*Member;
		return val;
	}

	template <typename Val, auto Member>
	void set_item(const Val &val) {
		T &comp = _handle.GetComponent<T>();
		Val &_val = comp.*Member;
		_val = val;
	}

	ComponentHandle(Ease::Entity &handle) {
		_handle = handle;
	}
};

void LuaScriptServer::RegisterECS() {
	_pState->new_usertype<Scene>(
		"Scene",

		"new", sol::no_constructor,

		"create",
		sol::overload(
			&Scene::Create,
			[](Ease::Scene &self, const std::string &name) -> Ease::Entity { return self.Create(name, 0U); }));

	_pState->new_enum(
		"Component",

		"Transform2D", Component::Component_Transform2D);

	_pState->new_usertype<ComponentHandle<Component::Transform2D>>(
		"Transform2D",

		"new", sol::no_constructor,

		"position",
		sol::property(
			&ComponentHandle<Component::Transform2D>::
				get_item<Ease::Vec2, &ComponentHandle<Component::Transform2D>::m_Position>,
			&ComponentHandle<Component::Transform2D>::
				set_item<Ease::Vec2, &ComponentHandle<Component::Transform2D>::m_Position>),

		"scale",
		sol::property(
			&ComponentHandle<Component::Transform2D>::
				get_item<Ease::Vec2, &ComponentHandle<Component::Transform2D>::m_Scale>,
			&ComponentHandle<Component::Transform2D>::
				set_item<Ease::Vec2, &ComponentHandle<Component::Transform2D>::m_Scale>),

		"rotation",
		sol::property(
			&ComponentHandle<Component::Transform2D>::
				get_item<float, &ComponentHandle<Component::Transform2D>::m_Rotation>,
			&ComponentHandle<Component::Transform2D>::
				set_item<float, &ComponentHandle<Component::Transform2D>::m_Rotation>));

	_pState->new_usertype<Entity>(
		"Entity",

		"valid", &Entity::IsValid,
		"name",
		sol::property(
			[](Entity &self) -> const std::string & { return self.GetComponent<Ease::Component::Common>().Name(); },
			[](Entity &self, const std::string &name) { self.GetComponent<Ease::Component::Common>().Name() = name; }),

		"has_component", [](Entity &self, int component) -> bool {
			if (!self.IsValid())
				return false;

			if (component == Component::Component_Transform2D) {
				return self.HasComponent<Component::Transform2D>();
			}
			return false; },

		"get_component", [](Entity &self, int component, sol::this_state L) -> sol::variadic_results {
			if (!self.IsValid())
				return false;

			sol::variadic_results res;

			if (component == Component::Component_Transform2D) {
				if(self.HasComponent<Component::Transform2D>()) {
					ComponentHandle<Component::Transform2D> comp(self);

					res.push_back( { L, sol::in_place_type<ComponentHandle<Component::Transform2D>>, comp } );
				}
			}

			if(res.size() == 0)
				res.push_back(sol::nil);

			return res; },

		"new", sol::no_constructor);
}
} // namespace Ease
