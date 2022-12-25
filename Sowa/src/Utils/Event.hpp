#ifndef _E_EVENT_HPP__
#define _E_EVENT_HPP__
#pragma once

#include "stlpch.hpp"

namespace Sowa {
template <typename T>
class Event {
  public:
	template <typename... Args>
	void Invoke(Args... args) {
		for (size_t i = 0; i < m_Functions.size(); i++)
			if (m_Functions[i])
				m_Functions[i](args...);
	}

	uint32_t operator+=(const std::function<T> &func) {
		m_IDCounter += 1;
		m_Functions[m_IDCounter] = func;
		return m_IDCounter;
	}
	void operator-=(uint32_t id) {
		if (m_Functions.count(id) > 0)
			m_Functions.erase(id);
	}

  private:
	std::map<uint32_t, std::function<T>> m_Functions;

	uint32_t m_IDCounter = 0;
};
} // namespace Sowa

#endif // _E_EVENT_HPP__