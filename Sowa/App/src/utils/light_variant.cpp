#include "light_variant.hpp"
#include "debug.hpp"

namespace sowa {
light_variant::light_variant() {
}
light_variant::light_variant(int v) {
	SetValue(v);
}
light_variant::light_variant(uint16_t v) {
	SetValue(v);
}
light_variant::light_variant(float v) {
	SetValue(v);
}
light_variant::light_variant(double v) {
	SetValue(v);
}
light_variant::light_variant(bool v) {
	SetValue(v);
}
light_variant::light_variant(const std::string &v) {
	SetValue(v);
}
light_variant::light_variant(const char *v) {
	SetValue(v);
}

light_variant::~light_variant() {
	RemoveValue();
}

light_variant::light_variant(const light_variant &v) {
	if (v.variantType == LightVariantType::Int) {
		SetValue(v.Int());
	} else if (v.variantType == LightVariantType::Float) {
		SetValue(v.Float());
	} else if (v.variantType == LightVariantType::Double) {
		SetValue(v.Double());
	} else if (v.variantType == LightVariantType::Bool) {
		SetValue(v.Bool());
	} else if (v.variantType == LightVariantType::String) {
		SetValue(v.String());
	}
}

int light_variant::Int() const {
	switch (variantType) {
	case LightVariantType::Int:
		return value.int_;
		break;
	case LightVariantType::UInt16:
		return static_cast<int>(value.uint16_);
		break;
	case LightVariantType::Float:
		return static_cast<int>(value.float_);
		break;
	case LightVariantType::Double:
		return static_cast<int>(value.double_);
		break;
	case LightVariantType::Bool:
		return value.bool_ ? 1 : 0;
		break;

	default:
		break;
	}
	return 0;
}
uint16_t light_variant::UInt16() const {
	switch (variantType) {
	case LightVariantType::Int:
		return static_cast<uint16_t>(value.int_);
		break;
	case LightVariantType::UInt16:
		return value.uint16_;
		break;
	case LightVariantType::Float:
		return static_cast<uint16_t>(value.float_);
		break;
	case LightVariantType::Double:
		return static_cast<uint16_t>(value.double_);
		break;
	case LightVariantType::Bool:
		return value.bool_ ? 1 : 0;
		break;

	default:
		break;
	}
	return 0;
}
float light_variant::Float() const {
	switch (variantType) {
	case LightVariantType::Int:
		return static_cast<float>(value.int_);
		break;
	case LightVariantType::UInt16:
		return static_cast<float>(value.uint16_);
		break;
	case LightVariantType::Float:
		return value.float_;
		break;
	case LightVariantType::Double:
		return static_cast<float>(value.double_);
		break;
	case LightVariantType::Bool:
		return value.bool_ ? 1.f : 0.f;
		break;

	default:
		break;
	}
	return 0.f;
}
double light_variant::Double() const {
	switch (variantType) {
	case LightVariantType::Int:
		return static_cast<double>(value.int_);
		break;
	case LightVariantType::UInt16:
		return static_cast<double>(value.uint16_);
		break;
	case LightVariantType::Float:
		return static_cast<double>(value.float_);
		break;
	case LightVariantType::Double:
		return value.double_;
		break;
	case LightVariantType::Bool:
		return value.bool_ ? 1.0 : 0.0;
		break;

	default:
		break;
	}
	return 0.0;
}
bool light_variant::Bool() const {
	switch (variantType) {
	case LightVariantType::Int:
		return value.int_ == 0 ? false : true;
		break;
	case LightVariantType::UInt16:
		return value.uint16_ == 0 ? false : true;
		break;
	case LightVariantType::Float:
		return static_cast<int>(value.float_) == 0 ? false : true;
		break;
	case LightVariantType::Double:
		return static_cast<int>(value.double_) == 0 ? false : true;
		break;
	case LightVariantType::Bool:
		return value.bool_;
		break;

	default:
		break;
	}
	return false;
}

std::string light_variant::String() const {
	if (variantType == LightVariantType::String) {
		return *value.string_;
	}
	return "";
}

void light_variant::SetValue(int v) {
	RemoveValue();

	variantType = LightVariantType::Int;
	value.int_ = v;
}
void light_variant::SetValue(uint16_t v) {
	RemoveValue();

	variantType = LightVariantType::UInt16;
	value.uint16_ = v;
}
void light_variant::SetValue(float v) {
	RemoveValue();

	variantType = LightVariantType::Float;
	value.float_ = v;
}
void light_variant::SetValue(double v) {
	RemoveValue();

	variantType = LightVariantType::Double;
	value.double_ = v;
}
void light_variant::SetValue(bool v) {
	RemoveValue();

	variantType = LightVariantType::Bool;
	value.bool_ = v;
}
void light_variant::SetValue(const std::string &v) {
	RemoveValue();

	variantType = LightVariantType::String;
	value.string_ = new std::string(v);
}
void light_variant::SetValue(const char *v) {
	RemoveValue();

	variantType = LightVariantType::String;
	value.string_ = new std::string(v);
}

void light_variant::RemoveValue() {
	if (variantType == LightVariantType::Int) {
		value.int_ = 0;
	} else if (variantType == LightVariantType::UInt16) {
		value.uint16_ = 0;
	} else if (variantType == LightVariantType::Float) {
		value.float_ = 0.f;
	} else if (variantType == LightVariantType::Double) {
		value.double_ = 0.0;
	} else if (variantType == LightVariantType::String) {
		delete value.string_;
		value.string_ = nullptr;
	}

	variantType = LightVariantType::None;
}

std::string light_variant::Repr() {
	switch (variantType) {
	case LightVariantType::Int:
		return std::to_string(value.int_);
		break;
	case LightVariantType::UInt16:
		return std::to_string(value.uint16_);
		break;
	case LightVariantType::Float:
		return std::to_string(value.float_);
		break;
	case LightVariantType::Double:
		return std::to_string(value.double_);
		break;
	case LightVariantType::Bool:
		return value.bool_ ? "true" : "false";
		break;
	case LightVariantType::String:
		return *value.string_;
		break;

	default:
		break;
	}
	return "";
}
} // namespace sowa
