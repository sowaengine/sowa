#include "filesystem.hpp"
#include "utils.hpp"

#if defined(SW_LINUX)
#include <unistd.h>
#elif defined(SW_WINDOWS)
#include <windows.h>
#endif

std::filesystem::path GetExecutableDir()
{
    #if defined(SW_LINUX)
        char path[FILENAME_MAX];
        ssize_t count = readlink("/proc/self/exe", path, FILENAME_MAX);
        return std::filesystem::path(std::string(path, (count > 0) ? count: 0)).parent_path().string();
    #elif defined(SW_WINDOWS)
        wchar_t path[FILENAME_MAX] = { 0 };
        GetModuleFileNameW(nullptr, path, FILENAME_MAX);
        return std::filesystem::path(path).parent_path().string();
    #endif
}

FILE* OpenExecutable(std::string path, std::vector<std::string> args) {
    std::cout << path << std::endl;

    FILE* handle = nullptr;
    std::string cmd = "";
    for(const std::string arg : args) {
        cmd += " \"" + arg + "\"";
    }
    std::cout << ": " << (path + cmd) << std::endl;
    #if defined (SW_LINUX)
        handle = popen((path + cmd).c_str(), "r");
    #elif defined (SW_WINDOWS)
        handle = popen((path + cmd).c_str(), "r");
    #endif
    return handle;
}

std::string GetLatestVersion(std::filesystem::path binPath) {
    std::filesystem::directory_entry latest{};
    uint32_t latestVersion = 0;

    for(const auto entry : std::filesystem::directory_iterator(binPath)) {
        if(entry.is_directory())
            continue;

        auto tokens = Split(entry.path().filename(), "-");
        if(tokens.size() == 0)
            continue;
        
        uint32_t versionNumber = 0;
        try {
            versionNumber = static_cast<uint32_t>(std::atoll(tokens[tokens.size()-1].c_str()));
        }
        catch(const std::exception& e) {
            continue;
        }
        
        if(versionNumber > latestVersion) {
            latestVersion = versionNumber;
            latest = entry;
        }
    }

    return latest.path().filename().string();
}

std::vector<std::filesystem::directory_entry> GetLocalVersions(std::filesystem::path binPath) {
    std::vector<std::filesystem::directory_entry> versions{};
    for(const auto entry : std::filesystem::recursive_directory_iterator(binPath)) {
        if(entry.is_directory())
            continue;

        versions.push_back(entry);
    }

    return versions;
}