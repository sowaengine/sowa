#include <iostream>
#include "../commands.hpp"
#include "../config.hpp"
#include "../utils.hpp"
#include "../console.hpp"

/*
    1.0  -----------> 1.0.0-stable
    0.1.1-a1-65809 -> 0.1.1-a1
*/
std::string resolveVersion(std::string version) {
    auto tokens = Split(version, ".");
    if(tokens.size() == 1) {
        return version + ".0.0-stable";
    }
    if(tokens.size() == 2) {
        return version + ".0-stable";
    }
    if(tokens.size() == 3) {
        auto rhsTokens = Split(tokens[2], "-");
        if(rhsTokens.size() == 1) {
            return version + "-stable";
        }
        if(rhsTokens.size() == 3) {
            //       0          .       1         .         1          .       a1
            return tokens[0] + "." + tokens[1] + "." + rhsTokens[0] + "-" + rhsTokens[1];
        }
    }
    
    return version;
}

void ActivateVersion(std::string version) {
    auto& cfg = GetConfig();
    if(version == "latest") {
        cfg.activeVersion = version;
    } else {
        size_t pos = version.find("/");
        if(pos == std::string::npos) {
            version = resolveVersion(version);
            cfg.activeVersion = "main/" + version;
        } else {
            std::string branch = version.substr(0, pos);
            version = version.substr(pos+1);
            version = resolveVersion(version);
            cfg.activeVersion = branch + "/" + version;
        }
    }
    std::cout << "Activated version: " << CYAN << cfg.activeVersion << RESET << std::endl;
    SaveConfig();
}