#ifndef _E_DIALOG_HPP__
#define _E_DIALOG_HPP__
#pragma once

#include "stlpch.hpp"

namespace sowa {
namespace Dialog {
/**
 * @brief Displays 'OpenFileDialog' popup
 *
 * @param title
 * @param defaultPath returns relative path to directory if not ""
 * @param filterCount
 * @param filterPatterns std::vector{"*.png", "file.png"}
 * @param multipleSelection
 * @param isRelative will return relative or absolute path
 * @param isInDirectory set to true if selected file is inside of defaultPath (can be in nested folders)
 * @return std::string path of selected file ("" if exited without selecting a file)
 */
std::string OpenFileDialog(const std::string &title,
						   const std::string &defaultPath,
						   int filterCount,
						   const std::vector<std::string> &filterPatterns,
						   bool multipleSelection,
						   bool isRelative,
						   bool *isInDirectory = nullptr);

} // namespace Dialog
} // namespace sowa

#endif // _E_DIALOG_HPP__