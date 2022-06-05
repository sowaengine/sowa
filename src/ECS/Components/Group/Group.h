#ifndef GROUP_H
#define GROUP_H

#pragma once
#include <string>
#include <vector>

namespace Ease::Component
{
    class Group
    {
    public:
        Group();
        ~Group();
        
        std::vector<std::string>& Groups() { return m_Groups; }
    private:
        std::vector<std::string> m_Groups;
    };
} // namespace Ease::Component

#endif