#ifndef RESOURCE_H
#define RESOURCE_H
#pragma once

#include <vector>
#include <string>

enum class PropertyType
{
	NONE = 0,
	STRING,
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

		std::vector<ResourceProperty> properties;
	public:
		Resource();
		~Resource();

};
#endif