#include "ui_size.hxx"
#include <algorithm>
#include <string>

#include <iostream>

UISize::UISize(const char *c) {
	SetValue(c);
}
UISize::UISize(const std::string &c) {
	SetValue(c.c_str());
}
void UISize::operator=(const char *c) {
	SetValue(c);
}
void UISize::operator=(const std::string &c) {
	SetValue(c.c_str());
}

void UISize::SetValue(const char *c) {
	std::string expr = c;
	// [numeric][scale]
	// 200px
	size_t substrIndex = 0;
	for (size_t i = 0; i < expr.size(); i++) {
		if (isdigit(expr[i]) || expr[i] == '.') {
			substrIndex++;
			continue;
		}

		break;
	}

	double v = std::stod(expr.substr(0, substrIndex));
	m_number = v;
	m_scale = expr.substr(substrIndex);
}