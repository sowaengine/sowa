#include "String.hpp"

namespace Ease
{
    std::vector<std::string> Split(const std::string& text, const std::string& delimiter)
    {
        std::vector<std::string> tokens;

        size_t start = 0U;
        size_t end = text.find(delimiter);
        while (end != std::string::npos)
        {
            tokens.emplace_back(text.substr(start, end - start));
            start = end + delimiter.length();
            end = text.find(delimiter, start);
        }
        tokens.emplace_back(text.substr(start, end));

        return tokens;
    }
} // namespace Ease
