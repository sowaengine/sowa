#include "Project.hpp"

#include <filesystem>
#include <iostream>

#include "Debug.hpp"

#include "Global.hpp"

namespace Ease
{



//static
void cl_Project::InitProject(const std::string& executableName)
{
   static cl_Project project;
   Global::Project = &project;
   Global::Project->Init(executableName);
}



void cl_Project::Init(const std::string& executableName)
{
   std::filesystem::path execPath(executableName);
   
   m_ProjectDir = std::filesystem::current_path();
   m_ExecutableName = execPath.filename().string();
   m_DataDir = m_ProjectDir + "/" + m_ExecutableName + "_Data";
   
   dataDir = m_DataDir;

   LOG("Data Dir: %s\n", m_DataDir.c_str())
}


std::string cl_Project::GetAbsolutePath(const std::string& relativePath)
{
   //assert(dataDir != "");
   return m_ProjectDir + "/" + m_ExecutableName + "_Data" + "/" + relativePath;
}
   
} // namespace Ease
