#ifndef LUASCRIPT_H
#define LUASCRIPT_H
#pragma once

#include <string>
#include "Resource.hpp"

class LuaScript : public Resource
{
	private:
		friend class Scene;
		std::string& m_Filepath;

		ResourceProperty prop_filepath;

		void PropertiesUpdated() override;
	public:
		LuaScript();
		LuaScript(const std::string& filepath);
		~LuaScript();

		void SetFilepath(const std::string& path);
		const std::string& GetFilepath();

};
#endif