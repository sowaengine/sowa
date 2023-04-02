#include <iostream>
#include "../commands.hpp"
#include "../config.hpp"
#include "../utils.hpp"
#include "../console.hpp"


void ActivateVersion(std::string version) {
    auto& cfg = GetConfig();
    if(version == "latest") {
        cfg.activeVersion = version;
    } else {
        size_t pos = version.find("/");
        if(pos == std::string::npos) {
            version = ResolveVersion(version);
            cfg.activeVersion = "main/" + version;
        } else {
            std::string branch = version.substr(0, pos);
            version = version.substr(pos+1);
            version = ResolveVersion(version);
            cfg.activeVersion = branch + "/" + version;
        }
    }
    std::cout << "Activated version: " << CYAN << cfg.activeVersion << RESET << std::endl;
    SaveConfig();
}