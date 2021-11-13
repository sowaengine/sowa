#pragma once

#include <string>

namespace Ease::File
{

// res:// returns project directory
std::string getPath_Head(const std::string& head);
void setPath_Head(const std::string& head, const std::string& path);


struct FileProperties
{
// on res://folder/path.png:
   // head is res
   // fullPath_Head is where res:// located (project folder)
   // body is folder/path.png

   std::string head;
   std::string fullPath_Head;
   std::string body;
   std::string fullPath;
};

FileProperties resolveFilepath(const std::string& path);
 
} // namespace Ease::File

