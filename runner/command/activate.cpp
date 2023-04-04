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
        std::string ver = ResolveFullVersion(version).Repr();
        cfg.activeVersion = ver;
    }
    std::cout << "Activated version: " << CYAN << cfg.activeVersion << RESET << std::endl;
    SaveConfig();
}