#include "File.hpp"

#include <map>
#include <iostream>
#include <filesystem>

namespace Ease
{
namespace File
{

std::map<std::string, std::string> paths;


std::string getPath_Head(const std::string& head)
{
   return paths[head];
}
void setPath_Head(const std::string& head, const std::string& path)
{
   std::string _path = std::filesystem::absolute(path);
   paths[head] = _path;
}



FileProperties resolveFilepath(const std::string& path)
{
   FileProperties props;

   std::string head = path.substr(0, path.find("://"));
   std::string body = path.substr( path.find("://")+3, path.size()-1 );

   props.head = head;
   props.fullPath_Head = getPath_Head(head);
   props.body = body;

   if(head == path)
   {
      props.fullPath = path;
   }
   else
      props.fullPath = props.fullPath_Head + props.body;

   return props;
}

} // namespace File  
} // namespace Ease
