#ifndef _E_COMMON_HPP__
#define _E_COMMON_HPP__

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