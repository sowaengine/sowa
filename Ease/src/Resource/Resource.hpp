#ifndef _E_RESOURCE_HPP__
#define _E_RESOURCE_HPP__

#pragma once

#include <stdint.h>
#include <string>

#include "Ease.hpp"

namespace Ease {

class BaseResource {
  public:
	ResourceID GetResourceID() { return m_ResourceID; }
	void SetResourceID(ResourceID id) { m_ResourceID = id; }

	void SetFilepath(const std::string &path) { _Filepath = path; }
	const std::string &GetFilepath() { return _Filepath; }

	BaseResource() = default;
	~BaseResource() = default;

  protected:
	ResourceID m_ResourceID{0};
	std::string _Filepath{""};
};

enum Resource_ : uint32_t {
	Resource_None = 0,
	Resource_ImageTexture
};

} // namespace Ease

#endif