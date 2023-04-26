#ifndef _E_EVENT_HPP__
#define _E_EVENT_HPP__
#pragma once

#include "stlpch.hpp"

namespace sowa {

typedef uint32_t EventRegister;

template <typename T>
class Event {
  public:
	template <typename... Args>
	void Invoke(Args... args) {
		for (size_t i = 0; i < m_Functions.size(); i++)
			if (m_Functions[i])
				m_Functions[i](args...);
	}

	EventRegister operator+=(const std::function<T> &func) {
		m_IDCounter += 1;
		m_Functions[m_IDCounter] = func;
		return m_IDCounter;
	}
	void operator-=(EventRegister id) {
		if (m_Functions.count(id) > 0)
			m_Functions.erase(id);
	}

  private:
	std::map<EventRegister, std::function<T>> m_Functions;

	EventRegister m_IDCounter = 0;
};
} // namespace sowa

#endif // _E_EVENT_HPP__