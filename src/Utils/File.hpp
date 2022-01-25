/**
 * @file File.hpp
 * @author Lexographics
 * @brief Utility functions about filesystem
 * @version 0.1
 * @date 2021-12-19
 */
#ifndef FILE_H
#define FILE_H
#pragma once
	
#include <string>

namespace Ease::Utils
{

   std::string GetFileContents(const char* filename);


} // namespace Ease::Utils
#endif