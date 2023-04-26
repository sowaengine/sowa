#ifndef SW_SIZE_HPP_
#define SW_SIZE_HPP_

#include "object_type.hpp"
#include "utils/file.hpp"

namespace sowa {
struct size : public object_type {
	int w;
	int h;

	static std::string Typename() { return "sowa::size"; }

	size() : w(0), h(0) { m_type = Typename(); }
	size(int w, int h) : w(w), h(h) { m_type = Typename(); }
	size &operator=(std::initializer_list<int> rhs) {
		m_type = Typename();
		if (rhs.size() != 2) {
			return *this;
		}

		auto it = rhs.begin();
		w = *it;
		++it;
		h = *it;

		return *this;
	}

	static FileBuffer SaveImpl(object_type *);
	static bool LoadImpl(object_type *out, const FileBuffer &doc);
};
} // namespace sowa

#endif