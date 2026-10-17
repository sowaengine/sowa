#pragma once

#include <string>
#include <utility>
#include <vector>

#include <flecs.h>

#include "core/reflection/field_type.hpp"
#include "ecs/component_type.hpp"
#include "ecs/ecs_types.hpp"
#include "ecs/field_registry.hpp"

namespace sowa {

// Registers native fields immediately through chained Field() calls. Undescribed
// members stay internal. Registration failure returns an inert builder with Error().
template <typename T> class ComponentBuilder {
  public:
	ComponentBuilder(flecs::world &world, ComponentType type, EcsError error)
		: _world(&world), _type(type), _error(error) {
	}

	// Describes member as a reflected field named name. Chainable.
	template <typename M>
	ComponentBuilder &Field(const std::string &name, M T::*member,
		FieldFlags flags = FieldFlags::Default) {
		static_assert(FieldTypeOf<M>::Valid,
			"member's type is not a supported component field type (see FieldType)");

		if (_error != EcsError::Ok) {
			return *this;
		}

		FieldDesc desc{name, FieldTypeOf<M>::Value, flags};
		if constexpr (FieldTypeOf<M>::Value == FieldType::Enum) {
			static_assert(EnumTraits<M>::Valid, "enum member has no SOWA_REFLECT_ENUM declaration");
			desc.enumName = EnumTraits<M>::Name();
			desc.enumConstants = EnumTraits<M>::Constants();
		}
		_descs.push_back(std::move(desc));
		AddComponentField(*_world, _type.Raw(), _descs.back(), FieldOffsetOf(member), true);
		// Rebuilt per field rather than once at the end, so the table is
		// always current and no terminal call is needed. Registration-time
		// only, and field counts are small.
		RefreshFieldTable(*_world, _type.Raw(), _descs);
		return *this;
	}

	template <typename M>
	ComponentBuilder &ResourceField(const std::string &name, M T::*member,
		const std::string &resourceType, FieldFlags flags = FieldFlags::Default) {
		static_assert(FieldTypeOf<M>::Value == FieldType::ResourceRef);
		Field(name, member, flags);
		if (_error == EcsError::Ok) {
			_descs.back().resourceType = resourceType;
			RefreshFieldTable(*_world, _type.Raw(), _descs);
		}
		return *this;
	}

	// The registered type; invalid if registration failed.
	ComponentType Type() const {
		return _type;
	}

	EcsError Error() const {
		return _error;
	}

	explicit operator bool() const {
		return _error == EcsError::Ok;
	}

	// Same value the data-described RegisterComponent overload returns, for
	// callers that want to handle both paths uniformly.
	EcsResult<ComponentType> Result() const {
		return EcsResult<ComponentType>{_type, _error};
	}

  private:
	flecs::world *_world = nullptr;
	ComponentType _type;
	EcsError _error = EcsError::Ok;
	std::vector<FieldDesc> _descs;
};

} // namespace sowa
