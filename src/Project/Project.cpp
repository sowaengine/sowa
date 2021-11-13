#include "Project.hpp"
#include "Ease.hpp"


namespace Ease
{

std::string Project::getPathFromExt(uint32_t ext_id)
{
   return externalPaths[ext_id];
}
   
} // namespace Ease
