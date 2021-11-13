#pragma once

#include "Ease.hpp"

namespace Ease
{

class Project;

class Serializer
{
public:
// Load project from file and load it into project
   static bool DeserializeProject(const std::string& filepath, Project& project);



private:
   
};

} // namespace Ease

