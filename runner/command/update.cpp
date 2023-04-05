#include "../commands.hpp"
#include <curl/curl.h>
#include <iostream>
#include <algorithm>

#include "tinyfiledialogs.h"

#include "../utils.hpp"
#include "../config.hpp"
#include "../console.hpp"
#include "../filesystem.hpp"

std::string GetURLOfServer(std::string branch) {
    for(auto s : GetConfig().servers) {
        if(s.name == branch) {
            return s.url;
        }
    }
    return "";
}

std::vector<Version> GetRemoteLatestVersions() {
    std::vector<VersionServer> servers;
    for(auto branch : GetConfig().activeBranches) {
        VersionServer s;
        s.name = branch;
        s.url = GetURLOfServer(branch);
        if(s.url != "") {
            servers.push_back(s);
        }
    }
    GetVersionsFromServer(servers);

    std::vector<Version> latestVersions;

    for(auto [branch, versions] : GetRuntimeConfig().versions) {
        if(versions.size() > 0) {
            std::sort(versions.begin(), versions.end());
            
            auto latest = versions[versions.size() - 1];
            latest.upstream = branch;
            latestVersions.emplace_back(latest);
        }
    }
    return latestVersions;
}

void CommandUpdate(std::filesystem::path appPath, bool showMessagebox) {
    std::cout << "Checking for updates...\n";

    auto versions = GetRemoteLatestVersions();
    if(versions.size() == 0) {
        return;
    }
    if(showMessagebox && tinyfd_messageBox("Update!", "Update available. Do you want to install?" , "yesno", "question", 1) != 1) {
        return;
    }
    for(auto ver : versions) {
        std::string appName = GetLatestVersion((appPath / "bin" / ver.upstream));
        
        BranchVersionPair localLatest = ResolveFullVersion(appName);
        VersionName localVer = VersionName(localLatest.version);
        
        std::cout << "\t" << ver.upstream << "/" << ver.tag;
        if(localVer == VersionName(ver.tag)) {
            std::cout << MAGENTA << " [Installed]" << RESET;
        }
        std::cout << ": " << CYAN << ver.url << RESET << std::endl;

        if(localVer < VersionName(ver.tag)) {
            InstallVersion(ver.tag, appPath);
            if(showMessagebox) {
                ActivateVersion(ver.tag);
                tinyfd_messageBox("Success", (std::string("Version ") + ver.upstream + "/" + ver.tag + " installed. "
                #ifdef SW_LINUX
                    "\n\trun $ chmod +x " + (appPath / "bin" / ver.upstream / ver.tag).string() + " to mark it as executable"
                #endif
                ).c_str(), "ok", "info", 0);
            }
        } else {
            std::cout << "\t" << GREEN << ver.upstream << RESET << " upstream is up to date " << MAGENTA << "(" << localLatest.Repr() << ")" << RESET << std::endl;
        }
    }
}