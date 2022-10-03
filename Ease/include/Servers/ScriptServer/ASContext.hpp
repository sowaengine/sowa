#ifndef _E_ASCONTEXT_HPP__
#define _E_ASCONTEXT_HPP__
#pragma once

#include "angelscript/angelscript.h"
#include <functional>
#include <string>

#include "Utils/Math.hpp"

namespace Ease {
class ScriptServerAS;
}
namespace Ease::ASContext {

class ASRefCounted {
  protected:
	ASRefCounted();
	~ASRefCounted();

  public:
	void _AddRef();
	void _Release();

	template <typename... Args>
	ASRefCounted(Args... args) {
		_RefCount = 1;
	}

  public:
	template <typename T, typename... Args>
	static T *_Create(Args... args) {
		return new T(args...);
	}

  private:
	int _RefCount{0};
};

class ASVector2 : public ASRefCounted {
  public:
	ASVector2();			 // Vec2(0, 0)
	ASVector2(float);		 // Vec2(s, s)
	ASVector2(float, float); // Vec2(x, y)
	ASVector2(const ASVector2 &rhs);
	ASVector2(const Ease::Vec2 &rhs);

	~ASVector2();

	ASVector2 &operator=(const ASVector2 &rhs);

	/// @see Utils/Math/Vec2
	float Length();
	ASVector2 *Clamp(float length = 1.0f);
	float Angle();

	float &x = vec.x;
	float &y = vec.y;

  public:
	static std::string _Repr(void *obj);

  private:
	friend class Ease::ScriptServerAS;
	Ease::Vec2 vec;
};

/// @brief Registers a representation function for given type name
void RegisterReprFunc(const std::string &_type, std::function<std::string(void *)> func);

/// @returns <INVALID {type}> if type repr doesn't exists
std::string GetObjectRepr(const std::string &_type, void *obj);
bool ObjectReprExists(const std::string &_type);

void Log_Generic(asIScriptGeneric *gen);
void Info_Generic(asIScriptGeneric *gen);
void Warn_Generic(asIScriptGeneric *gen);
void Error_Generic(asIScriptGeneric *gen);

} // namespace Ease::ASContext

#endif // _E_ASCONTEXT_HPP__