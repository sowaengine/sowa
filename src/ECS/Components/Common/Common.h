#ifndef COMMON_H
#define COMMON_H

#pragma once
#include <string>

namespace Ease::Component
{
    class Common
    {
    public:
        Common();
        ~Common();
        
        std::string& Name() { return m_Name; }
    private:
        std::string m_Name;
    };
} // namespace Ease::Component

#endif