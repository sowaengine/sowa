#ifndef RESOURCE_H
#define RESOURCE_H
#pragma once

#include <vector>
#include <string>

enum class PropertyType
{
	NONE = 0,
	STRING,
	SCRIPT,
	TEXTURE,
};
struct ResourceProperty
{
	std::string name;
	PropertyType type;
	
	std::string dataStr;


	ResourceProperty() {}
	ResourceProperty(PropertyType _type) : type(_type) {}
	~ResourceProperty() {}
};

class Resource  
{
	protected:
		friend class Editor;
		friend class ResourceManager;
		friend class Scene;

		std::vector<ResourceProperty*> properties;

		uint32_t m_ResourceID;
		std::string m_Name;

		virtual void PropertiesUpdated() {} // Called when any property has changed
	public:
		Resource();
		~Resource();

		uint32_t GetID() { return m_ResourceID; }

};
#endif