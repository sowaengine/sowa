#include "Resource.hpp"  
#include "ResourceManager.hpp"

Resource::Resource()
{
	
}
	
Resource::~Resource()
{
	ResourceManager::get_singleton().RemoveResource(m_ResourceID);
}