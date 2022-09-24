#ifndef _E_ENGINECONTEXT_HPP__
#define _E_ENGINECONTEXT_HPP__

#include <iostream>
#include <string>
#include <unordered_map>

namespace Ease {
class EngineContext {
  public:
	template <typename T>
	void RegisterSingleton(const std::string &name, T &singleton) {
		_singletons[name] = reinterpret_cast<void *>(&singleton);
	}

	template <typename T>
	T *GetSingleton(const std::string &name) {
		if (_singletons.count(name) != 1) {
			std::cout << "Singleton '" << name << "' doesn't exists" << std::endl;
			return nullptr;
		}

		T *singleton = reinterpret_cast<T *>(_singletons[name]);
		return singleton;
	}

	static EngineContext *CreateContext() {
		return new EngineContext;
	}

  private:
	std::unordered_map<std::string, void *> _singletons{};

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