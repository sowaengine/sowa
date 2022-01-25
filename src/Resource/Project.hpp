/**
 * @file Project.hpp
 * @author Lexographics
 * @brief 
 * @version 0.1
 * @date 2022-01-13
 */
#ifndef PROJECT_H
#define PROJECT_H
#pragma once

#include "ProjectSettings.hpp"
#include <filesystem>

/**
 * @brief Can only have one instance
 */
class Project
{
	private:
		Project();


		std::filesystem::directory_entry m_ProjectDir;
	public:
		// Methods
		static Project& get_singleton();

		
		void LoadProject(const std::string& projectDir);

		std::string GetAbsolutePath(const std::string& relativePath);

		



		// Members
		ProjectSettings projectSettings;



		~Project();
};
#endif