#include "../commands.hpp"
#include "../utils.hpp"
#include "../console.hpp"
#include "../net.hpp"
#include <iostream>
#include <fstream>

void InstallVersion(std::string version) {
    std::string branch = "main";
    if(version != "latest") {
        size_t pos = version.find("/");
        if(pos == std::string::npos) {
            version = ResolveVersion(version);
        } else {
            branch = version.substr(0, pos);
            version = version.substr(pos+1);
            version = ResolveVersion(version);
        }
    }
    std::cout << "Insalling version: " << CYAN << branch << "/" << version << RESET << std::endl;
    GetVersionsFromServer();
    for(auto v : GetRuntimeConfig().versions[branch]) {
        if (version == v.tag) {
            std::cout << "Installing from " << v.url << std::endl;
            std::vector<unsigned char> file = GetFileHTTP(v.url);

            
            
        }
    }
}