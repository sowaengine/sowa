#ifndef _E_ASCONTEXT_HPP__
#define _E_ASCONTEXT_HPP__
#pragma once

#include "Core/Application.hpp"
#include "Servers/GuiServer/GuiServer.hpp"
#include "angelscript.h"
#include <functional>
#include <string>

#include "ECS/Entity/Entity.hpp"
#include "Utils/Math.hpp"

namespace Ease {
class ScriptServerAS;
class Window;
class GuiServer;
} // namespace Ease
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

// Bridge class to communicate with Ease::Window class
class WindowCaller {
  public:
	WindowCaller(Ease::Window *window);
	~WindowCaller();

	ASVector2 *GetWindowSize();

  private:
	Ease::Window *_pWindow{nullptr};
};

// Bridge class to communicate with Ease::Window class
class GuiCaller {
  public:
	GuiCaller(Ease::GuiServer *gui);
	~GuiCaller();

	void PushStyleVar(StyleVar var, ASVector2 *vec);

  private:
	Ease::GuiServer *_Gui{nullptr};
};

// reference counted object with handle to Ease::Entity
class ASEntity : public ASRefCounted {
  public:
	ASEntity();
	~ASEntity();

	ASEntity &operator=(const ASEntity &rhs);
	void Echo();

  private:
	friend class ApplicationCaller;
	friend class ASScene;
	Ease::Entity _Handle;
};

// reference counted object with handle to Ease::Scene
class ASScene : public ASRefCounted {
  public:
	ASScene();
	ASScene(ASScene *scn);
	~ASScene();

	ASScene &operator=(const ASScene &rhs);

	ASEntity *Create(const std::string &name, uint32_t id = 0);

	// not exposed to script api
	ASScene(Reference<Ease::Scene> _scene);

	bool Valid();

  private:
	friend class ApplicationCaller;
	Reference<Ease::Scene> _Handle;
};

class ApplicationCaller {
  public:
	ApplicationCaller(Ease::Application *app);
	~ApplicationCaller();

	ASScene *GetCurrentScene();

  private:
	Ease::Application *_App{nullptr};
};

} // namespace Ease::ASContext

#endif // _E_ASCONTEXT_HPP__