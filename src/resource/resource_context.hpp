#pragma once

namespace sowa {

class FileSystem;
class GraphicsDevice;
class ResourceRegistry;

// Services passed by ResourceRegistry to Reload and Unload. Resources are default-
// constructed and acquire derived state later. Every pointer may be null; check before use.
struct ResourceContext {
	GraphicsDevice *device = nullptr;
	FileSystem *fs = nullptr;
	ResourceRegistry *registry = nullptr;
};

} // namespace sowa
