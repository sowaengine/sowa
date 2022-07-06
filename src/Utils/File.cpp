#include "File.hpp"

#include <iostream>
#include "String.hpp"
#include <unordered_map>

namespace Ease::File
{
    // res:// -> res
    std::unordered_map<std::string, std::filesystem::path> File_path_endpoints;

    std::filesystem::path Path(const std::filesystem::path& path, FilePathData* pathdata)
    {
        std::vector<std::string> tokens = Ease::Split(path.string(), "://");
        if(tokens.size() != 2)
        {
            std::cout << "Invalid Path: " << path << std::endl;
            return "";
        }
        std::string endpoint = tokens[0];
        std::filesystem::path relativePath = tokens[1];
        
        if(File_path_endpoints.count(endpoint) == 0)
        {
            std::cout << "Endpoint " << tokens[0] << " not found: " << path << std::endl;
            return "";
        }
        std::filesystem::path realPath = File_path_endpoints[endpoint] / relativePath;
        if(pathdata)
        {
            pathdata->endpoint = endpoint;
            pathdata->relativePath = relativePath;
        }
        return realPath;
    }



    bool InsertFilepathEndpoint(const std::string& endpoint, const std::filesystem::path& path, bool force)
    {
        if(!force && File_path_endpoints.count(endpoint) != 0)
            return false;
        
        File_path_endpoints[endpoint] = path;
        return true;
    }
} // namespace Ease::File
