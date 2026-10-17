#pragma once

namespace sowa {

// Marks runtime-described component types.
// ReplaceDataDescribedFields requires this marker to protect native layouts.
struct DataDescribed {};

// Marks component instances whose _start ran and whose _exit may run. Unregistered
// and preserved through field-layout migration.
struct ScriptLive {};

} // namespace sowa
