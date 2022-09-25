#ifndef _E_ENGINECONTEXT_HPP__
#define _E_ENGINECONTEXT_HPP__

#include "Ease.hpp"
#include <iostream>
#include <string>
#include <unordered_map>

namespace Ease {
class EngineContext {
  public:
	template <typename T>
	void RegisterSingleton(Ease::Server id, T &singleton) {
		_singletons[id] = reinterpret_cast<void *>(&singleton);
	}

	template <typename T>
	T *GetSingleton(Ease::Server id) {
		if (_singletons.count(id) != 1) {
			std::cout << "Singleton '" << (uint32_t)id << "' doesn't exists" << std::endl;
			return nullptr;
		}

		T *singleton = reinterpret_cast<T *>(_singletons[id]);
		return singleton;
	}

	static EngineContext *CreateContext() {
		return new EngineContext;
	}

  private:
	std::unordered_map<Ease::Server, void *> _singletons{};

  private:
	EngineContext();
	virtual ~EngineContext();

	EngineContext(const EngineContext &) = delete;
	EngineContext(const EngineContext &&) = delete;
	EngineContext &operator=(const EngineContext &) = delete;
	EngineContext &operator=(const EngineContext &&) = delete;
};
} // namespace Ease

#endif // _E_ENGINECONTEXT_HPP__