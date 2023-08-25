#ifndef SW_RESOURCE_HXX
#define SW_RESOURCE_HXX
#pragma once

#include <any>
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "resource_type.hxx"

class Resource {
  public:
	inline RID ResourceID() { return m_resourceID; }
	inline const std::string &Filepath() { return m_filepath; }
	inline ResourceType Type() { return m_type; }

	inline std::any &Data() { return m_data; }

	template <typename T>
	T *As() {
		T *data = std::any_cast<T>(&m_data);
		return data;
	}

  protected:
	friend class ResourceManager;

	// Resource Data (ImageTexture, Font...)
	std::any m_data;
	RID m_resourceID = 0;

	std::string m_filepath;
	ResourceType m_type;
};

#endif // SW_RESOURCE_HXX