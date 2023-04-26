#ifndef SW_OBJECT_TYPE_HPP_
#define SW_OBJECT_TYPE_HPP_

#include "stlpch.hpp"

namespace sowa {

/**
 * Base object type
 */
class object_type {
  public:
	virtual ~object_type();
	const std::string get_type() const { return m_type; }

  protected:
	object_type();
	std::string m_type{"Object"};
};
} // namespace sowa

#endif // SW_OBJECT_TYPE_HPP_