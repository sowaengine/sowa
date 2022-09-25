#ifndef _E_ASCONTEXT_HPP__
#define _E_ASCONTEXT_HPP__
#pragma once

#include "angelscript/angelscript.h"
#include <functional>
#include <string>

#include "Utils/Math.hpp"

namespace Ease::ASContext {

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