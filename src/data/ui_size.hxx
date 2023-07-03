#ifndef SW_UI_SIZE
#define SW_UI_SIZE
#pragma once

#include <string>

struct UISize {
  public:
	UISize() = default;
	UISize(const char *c);
	UISize(const std::string &c);
	void operator=(const char *c);
	void operator=(const std::string &c);

	void SetValue(const char *c);

	inline double &Number() { return m_number; }
	inline std::string &Scale() { return m_scale; }

  private:
	double m_number;
	std::string m_scale;
};

#endif // SW_UI_SIZE