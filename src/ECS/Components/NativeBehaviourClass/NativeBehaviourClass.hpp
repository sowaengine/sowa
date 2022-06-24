#ifndef NATIVEBEHAVIOURCLASS_H
#define NATIVEBEHAVIOURCLASS_H

#pragma once

#include <string>
#include "Ease.hpp"

namespace Ease::Component
{
    class NativeBehaviourClass
    {
    public:
        NativeBehaviourClass();
        ~NativeBehaviourClass();

        std::string& ClassName() { return m_ClassName; }
        Ease::NativeBehaviour*& Behaviour() { return pBehaviour; } 
        Ease::NativeBehaviourFactory*& Factory() { return pFactory; } 
    
    private:
        std::string m_ClassName = "";

        Ease::NativeBehaviourFactory* pFactory = nullptr;
        Ease::NativeBehaviour* pBehaviour = nullptr;
    };
}

#endif