#include "Servers/ScriptServer/ASContext.hpp"
#include "Core/Application.hpp"
#include "Core/Window.hpp"
#include "Debug.hpp"
#include "ECS/Entity/Entity.hpp"
#include "ECS/Scene/Scene.hpp"
#include "Servers/GuiServer/GuiServer.hpp"
#include <functional>
#include <string>
#include <unordered_map>

#include "fmt/args.h"
#include "fmt/core.h"

static void Print_Generic(asIScriptGeneric *gen, const std::string &levelText);

namespace Ease::ASContext {

//* ASRefCounted */
ASRefCounted::ASRefCounted()
	: _RefCount(1) {}
ASRefCounted::~ASRefCounted() {}

void ASRefCounted::_AddRef() {
	_RefCount++;
}
void ASRefCounted::_Release() {
	if (--_RefCount == 0) {
		delete this;
	}
}

//* ASVector2 */
ASVector2::ASVector2()
	: vec(0.f, 0.f) {}
ASVector2::ASVector2(float s)
	: vec(s, s) {}
ASVector2::ASVector2(float x, float y)
	: vec(x, y) {}
ASVector2::ASVector2(const ASVector2 &rhs)
	: vec(rhs.x, rhs.y) {}
ASVector2::ASVector2(const Ease::Vec2 &rhs)
	: vec(rhs.x, rhs.y) {}

ASVector2::~ASVector2() {}

ASVector2 &ASVector2::operator=(const ASVector2 &rhs) {
	x = rhs.x;
	y = rhs.y;
	return *this;
}

float ASVector2::Length() {
	return vec.Length();
}
ASVector2 *ASVector2::Clamp(float length /* = 1.0f*/) {
	return ASContext::ASRefCounted::_Create<ASVector2>(vec.Clamp(length));
}
float ASVector2::Angle() {
	return vec.Angle();
}

// static
std::string ASVector2::_Repr(void *obj) {
	ASVector2 *v = static_cast<ASVector2 *>(obj);

	return fmt::format("Vector2({}, {})", v->x, v->y);
}

static std::unordered_map<std::string, std::function<std::string(void *)>> s_ReprFuncs;
void RegisterReprFunc(const std::string &_type, std::function<std::string(void *)> func) {
	s_ReprFuncs[_type] = func;
}
std::string GetObjectRepr(const std::string &_type, void *obj) {
	if (!ObjectReprExists(_type)) {
		Debug::Error("AS Object Repr function with type {} doesn't exists!", _type);
		return fmt::format("<INVALID {}>", _type);
	}
	return s_ReprFuncs[_type](obj);
}
bool ObjectReprExists(const std::string &_type) {
	return !(s_ReprFuncs.count(_type) == 0 && s_ReprFuncs[_type] != nullptr);
}

void Log_Generic(asIScriptGeneric *gen) {
	Print_Generic(gen, "LOG");
}
void Info_Generic(asIScriptGeneric *gen) {
	Print_Generic(gen, "INFO");
}
void Warn_Generic(asIScriptGeneric *gen) {
	Print_Generic(gen, "WARN");
}
void Error_Generic(asIScriptGeneric *gen) {
	Print_Generic(gen, "ERROR");
}

// Window
WindowCaller::WindowCaller(Ease::Window *window) : _pWindow{window} {}
WindowCaller::~WindowCaller() {}

ASVector2 *WindowCaller::GetWindowSize() {
	return ASRefCounted::_Create<ASVector2>((float)_pWindow->GetWindowWidth(), (float)_pWindow->GetWindowHeight());
}

// void PushStyleVar(ASVector2 *vec);
GuiCaller::GuiCaller(Ease::GuiServer *gui) : _Gui{gui} {}
GuiCaller::~GuiCaller() {}

void GuiCaller::PushStyleVar(StyleVar var, ASVector2 *vec) {
	if (vec == nullptr)
		return;

	_Gui->PushStyleVar(var, Vec2(vec->x, vec->y));
}

ApplicationCaller::ApplicationCaller(Ease::Application *app) : _App{app} {}
ApplicationCaller::~ApplicationCaller() {}

ASScene *ApplicationCaller::GetCurrentScene() {
	return ASRefCounted::_Create<ASScene>(_App->GetCurrentScene());
}

ASEntity &ASEntity::operator=(const ASEntity &rhs) {
	_Handle = rhs._Handle;
	return *this;
}
void ASEntity::Echo() {
	Debug::Log("Echo");
}

ASScene::ASScene() {
	_Handle = std::make_shared<Ease::Scene>();
}
ASScene::ASScene(ASScene *scn) {
	_Handle = scn->_Handle;
}
ASScene::~ASScene() {}

ASEntity::ASEntity() {}
ASEntity::~ASEntity() {}

ASScene &ASScene::operator=(const ASScene &rhs) {
	_Handle = rhs._Handle;
	return *this;
}

ASEntity *ASScene::Create(const std::string &name, uint32_t id /*= 0*/) {
	ASEntity *entity = ASRefCounted::_Create<ASEntity>();
	entity->_Handle = _Handle->Create(name, id);
	return entity;
}

ASScene::ASScene(Reference<Ease::Scene> _scene) : _Handle(_scene) {}

bool ASScene::Valid() {
	return _Handle != nullptr;
}

} // namespace Ease::ASContext

void Print_Generic(asIScriptGeneric *gen, const std::string &levelText) {
	if (gen->GetArgCount() == 0) {
		Debug::Error("Debug::Log and equivalent functions must take more than 0 args.");
		return;
	}

	std::string format = *(std::string *)gen->GetArgAddress(0);

	fmt::dynamic_format_arg_store<fmt::format_context> store;

	for (size_t i = 1; i < gen->GetArgCount(); i++) {
		void *ref = gen->GetArgAddress(i);
		int typeID = gen->GetArgTypeId(i);

		if (typeID == asTYPEID_VOID) {
			store.push_back("null");
			continue;
		}
		if (typeID == asTYPEID_INT32) {
			store.push_back(*(int *)ref);
			continue;
		}
		if (typeID == asTYPEID_FLOAT) {
			store.push_back(*(float *)ref);
			continue;
		}

		asITypeInfo *tinfo = gen->GetEngine()->GetTypeInfoById(typeID);
		std::string tname = tinfo->GetName();

		// if (Ease::ASContext::ObjectReprExists(tname)) {
		std::string repr = Ease::ASContext::GetObjectRepr(tname, ref);
		store.push_back(repr);
		//} else
		//	store.push_back("<unknown-value>");
	}

	std::string msg = "FORMAT ERROR!";
	try {
		msg = fmt::vformat(format, store);
	} catch (const std::exception &e) {
		Debug::Error("String Formatting error: {}", e.what());
	}

	if (levelText == "LOG")
		Debug::Log(msg);
	else if (levelText == "INFO")
		Debug::Info(msg);
	else if (levelText == "WARN")
		Debug::Warn(msg);
	else if (levelText == "ERROR")
		Debug::Error(msg);
}