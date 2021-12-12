#include "Project.hpp"

#include <filesystem>
#include <iostream>

#include "Debug.hpp"

namespace Ease
{

void cl_Project::InitProject(const std::string& executableName)
{
   std::filesystem::path execPath(executableName);
   
   m_ProjectDir = std::filesystem::current_path();
   m_ExecutableName = execPath.filename();
   m_DataDir = m_ProjectDir.concat("/" + m_ExecutableName + "_Data");
   
   

   LOG("Data Dir: %s\n", m_DataDir.string().c_str())
}


std::filesystem::path cl_Project::GetAbsolutePath(const std::string& relativePath)
{
   return m_DataDir / relativePath;
}
   
} // namespace Ease
