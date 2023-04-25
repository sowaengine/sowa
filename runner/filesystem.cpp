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
    return "";
}

FILE* OpenExecutable(std::string path, std::vector<std::string> args) {
    FILE* handle = nullptr;
    std::string cmd = "";
    for(const std::string arg : args) {
        cmd += " \"" + arg + "\"";
    }
    std::cout << (path + cmd) << std::endl;
    #if defined (SW_LINUX)
        handle = popen((path + cmd).c_str(), "r");
    #elif defined (SW_WINDOWS)
        handle = popen((path + cmd).c_str(), "r");
    #endif
    return handle;
}

std::string GetLatestVersion(std::filesystem::path binPath) {
    std::filesystem::directory_entry latest{};
    uint32_t latestHash = 0;

    for(const auto entry : std::filesystem::directory_iterator(binPath)) {
        if(entry.is_directory())
            continue;
        if(entry.path().filename().extension() == ".exe") {
            entry.path().filename().replace_extension("");
        }
        
        BranchVersionPair ver = ResolveFullVersion(entry.path().filename().string());
        uint32_t hash = VersionName(ver.version).Hash();
        
        if(hash > latestHash) {
            latestHash = hash;
            latest = entry;
        }
    }

    return latest.path().filename().string();
}

std::string GetVersionInPath(std::filesystem::path binPath, std::string version) {
    for(const auto entry : std::filesystem::directory_iterator(binPath)) {
        if(entry.is_directory())
            continue;
        
        if(StartsWith(entry.path().filename().string(), std::string("sowa-") + ResolveVersion(version) + "-")) {
            return entry.path().filename().string();
        }
    }
    return "";
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