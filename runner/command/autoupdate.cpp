#include "../commands.hpp"
#include "../config.hpp"
#include "../net.hpp"
#include "../utils.hpp"

void AutoUpdate() {
    for(const std::string& s : GetConfig().activeBranches) {
        std::string content = GetContentHTTP(s);
        std::vector<Version> versions = ParseVersions(content);
    }
}