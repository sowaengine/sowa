#ifndef _E_DIALOG_HPP__
#define _E_DIALOG_HPP__
#pragma once

#include <string>
#include <vector>

namespace Ease::Dialog
{
    /**
     * @brief Displays 'OpenFileDialog' popup
     * 
     * @param title
     * @param defaultPath 
     * @param filterCount 
     * @param filterPatterns std::vector{"*.png", "file.png"}
     * @param multipleSelection 
     * @return std::string path of selected file ("" if exited without selecting a file)
     */
    std::string OpenFileDialog(const std::string& title,
                const std::string& defaultPath,
                int filterCount,
                const std::vector<std::string>& filterPatterns,
                bool multipleSelection);

} // namespace Ease::Dialog


#endif // _E_DIALOG_HPP__