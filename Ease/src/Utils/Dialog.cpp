#include "Utils/Dialog.hpp"
#include "tinyfiledialogs.h"

#include <string>
#include <vector>

namespace Ease
{
    std::string Dialog::OpenFileDialog(
        const std::string& title,
        const std::string& defaultPath,
        int filterCount,
        const std::vector<std::string>& filterPatterns,
        bool multipleSelection)
    {
        char* path = tinyfd_openFileDialog(title.c_str(), defaultPath.c_str(), filterCount, (const char* const*)filterPatterns.data(), NULL, multipleSelection ? 1 : 0);
        if(path == nullptr)
            return "";
        
        return std::string{path};
    }

} // namespace Ease
