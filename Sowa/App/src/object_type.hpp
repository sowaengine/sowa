#ifndef SW_OBJECT_TYPE_HPP_
#define SW_OBJECT_TYPE_HPP_

#include "stlpch.hpp"

namespace Sowa {

/**
 * Base object type
 */
class ObjectType {
  public:
	virtual ~ObjectType();
	const std::string GetType() const { return m_Type; }

  protected:
	ObjectType();
	std::string m_Type{"Object"};
};
} // namespace Sowa

#endif // SW_OBJECT_TYPE_HPP_