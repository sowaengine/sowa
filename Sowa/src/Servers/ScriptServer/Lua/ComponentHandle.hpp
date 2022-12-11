#ifndef _E_COMPONENT_HANDLE_HPP__
#define _E_COMPONENT_HANDLE_HPP__

#pragma once

#include "ECS/Entity/Entity.hpp"

class ComponentBaseHandle {
  protected:
	Sowa::Entity _handle;
};

template <typename T>
class ComponentHandle : public T, public ComponentBaseHandle {
  public:
	template <typename Val, auto Member>
	Val &get_item() {
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

	ComponentHandle(Sowa::Entity &handle) {
		_handle = handle;
	}

	Sowa::Entity &Handle() { return _handle; }
};

#endif