#ifndef _E_FILE_HPP__
#define _E_FILE_HPP__
#pragma once

#include <string>
#include <filesystem>

namespace Ease::File
{
    struct FilePathData
    {
        std::filesystem::path relativePath;
        std::filesystem::path endpoint;
    };
    /**
     * @brief Returns real path from given string like res://path/file -> ./ProjectPath/path/file
     * 
     * @param path path with delimiter (like res://)
     * @param pathdata fill information about path if given
     * @return std::string real path
     */
    std::filesystem::path Path(const std::filesystem::path& path, FilePathData* pathdata = nullptr);

    /**
     * @brief Add Custom Endpoint
     * 
     * @param path should be a directory
     * @param force modify endpoint if it already exists
     * @return true if endpoint inserted
     * @return false if endpoint already exists
     */
    bool InsertFilepathEndpoint(const std::string& endpoint, const std::filesystem::path& path, bool force = false);

} // namespace Ease::File


#endif // _E_FILE_HPP__