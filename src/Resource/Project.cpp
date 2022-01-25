#include "Project.hpp"  

#include <filesystem>
#include "Debug.hpp"


Project::Project()
{
}
	
Project::~Project()
{
}

// static 
Project& Project::get_singleton() 
{
   static Project proj;
   return proj;
}

void Project::LoadProject(const std::string& projectDir) 
{
   std::filesystem::directory_entry projDir(projectDir);

   if(!projDir.exists())
   {
      LOG("Failed to load project %s", projectDir.c_str());
      throw std::runtime_error("");
   }

   m_ProjectDir = projDir;
}


std::string Project::GetAbsolutePath(const std::string& relativePath)
{
   //assert(dataDir != "");
   LOG("%s", std::string(m_ProjectDir.path().string() + "/" + relativePath).c_str() );
   return m_ProjectDir.path().string() + "/" + relativePath;
}
