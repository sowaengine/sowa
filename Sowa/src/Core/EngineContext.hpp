#ifndef _E_ENGINECONTEXT_HPP__
#define _E_ENGINECONTEXT_HPP__

#include "Debug.hpp"
#include "Sowa.hpp"
#include <iostream>
#include <string>
#include <unordered_map>

namespace Sowa {
class EngineContext {
  public:
	template <typename T>
	void RegisterSingleton(Sowa::Server id, T &singleton) {
		_singletons[id] = reinterpret_cast<void *>(&singleton);
	}

	template <typename T>
	T *GetSingleton(Sowa::Server id) {
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
	std::unordered_map<Sowa::Server, void *> _singletons{};

  private:
	EngineContext();
	virtual ~EngineContext();

	EngineContext(const EngineContext &) = delete;
	EngineContext(const EngineContext &&) = delete;
	EngineContext &operator=(const EngineContext &) = delete;
	EngineContext &operator=(const EngineContext &&) = delete;
};
} // namespace Sowa

#endif // _E_ENGINECONTEXT_HPP__