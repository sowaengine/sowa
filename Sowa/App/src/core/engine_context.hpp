#ifndef _E_ENGINECONTEXT_HPP__
#define _E_ENGINECONTEXT_HPP__

#include "debug.hpp"
#include "sowa.hpp"
#include "stlpch.hpp"

namespace sowa {
class EngineContext {
  public:
	template <typename T>
	void RegisterSingleton(sowa::Server id, T &singleton) {
		_singletons[id] = reinterpret_cast<void *>(&singleton);
	}

	template <typename T>
	T *GetSingleton(sowa::Server id) {
		if (_singletons.count(id) != 1) {
			Debug::Error("Singleton '{}' doesn't exists", (uint32_t)id);
			return nullptr;
		}

		T *singleton = reinterpret_cast<T *>(_singletons[id]);
		return singleton;
	}

	static EngineContext *CreateContext() {
		return new EngineContext;
	}

  private:
	std::unordered_map<sowa::Server, void *> _singletons{};

  private:
	EngineContext();
	virtual ~EngineContext();

	EngineContext(const EngineContext &) = delete;
	EngineContext(const EngineContext &&) = delete;
	EngineContext &operator=(const EngineContext &) = delete;
	EngineContext &operator=(const EngineContext &&) = delete;
};
} // namespace sowa

#endif // _E_ENGINECONTEXT_HPP__