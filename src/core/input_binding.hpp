#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/input.hpp"
#include "core/reflection/enum_constant.hpp"
#include "core/types.hpp"

namespace sowa {

// Text form of InputBinding (declared in core/input.hpp).

// Every named key, shared by binding text and the Lua Key table. Where a
// key has aliases (MouseLeft/Mouse1), the preferred name comes first and is
// the one FormatInputBinding writes. Does not include Key::Unknown.
const std::vector<EnumConstant> &KeyConstants();

// Empty for a key without a name.
std::string_view KeyName(Key key);
std::optional<Key> KeyFromName(std::string_view name);

// The modifier bit a modifier key holds down (LeftControl -> Control), or 0.
u8 ModifierOfKey(Key key);

// "Ctrl+Shift+S" -> {S, Control | Shift}. Names are case-sensitive and match
// KeyConstants; modifiers are Ctrl, Shift, Alt and Super, in any order.
// Returns nullopt for unknown names, repeated modifiers or a missing key.
std::optional<InputBinding> ParseInputBinding(std::string_view text);

// Canonical text: modifiers in Ctrl, Shift, Alt, Super order, then the key.
// A binding whose key has no name formats as an empty string.
std::string FormatInputBinding(const InputBinding &binding);

} // namespace sowa
