#ifndef _E_DIALOG_HPP__
#define _E_DIALOG_HPP__
#pragma once

#include <string>
#include <vector>

namespace Ease {
namespace Dialog {
/**
 * @brief Displays 'OpenFileDialog' popup
 *
 * @param title
 * @param defaultPath returns relative path to directory if not ""
 * @param filterCount
 * @param filterPatterns std::vector{"*.png", "file.png"}
 * @param multipleSelection
 * @param isInDirectory set to true if selected file is inside of defaultPath (can be in nested folders)
 * @return std::string path of selected file ("" if exited without selecting a file)
 */
std::string OpenFileDialog(const std::string &title,
						   const std::string &defaultPath,
						   int filterCount,
						   const std::vector<std::string> &filterPatterns,
						   bool multipleSelection,
						   bool *isInDirectory = nullptr);

} // namespace Dialog
} // namespace Ease

#endif // _E_DIALOG_HPP__