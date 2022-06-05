#ifndef NATIVEBEHAVIOUR_H
#define NATIVEBEHAVIOUR_H

#pragma once

#include <stdint.h>
#include "Ease.hpp"
#include "Resource/Resource.h"
#include "Ease.hpp"
#include <functional>

typedef uint32_t ResourceID;

namespace Ease
{
    class NativeBehaviour : public BaseResource
    {
    public:
        NativeBehaviour();
        ~NativeBehaviour();
        
        void CallStart();
        void CallUpdate();

        inline void SetBehaviour(BaseBehaviour* behaviour) { m_pBehaviour = behaviour;}
        inline BaseBehaviour* GetBehaviour() { return m_pBehaviour; }

        void SetDeleteFunc(std::function<void(BaseBehaviour*)> func) { m_DeleteFunc = func; }
    private:
        friend class Application;
        BaseBehaviour* m_pBehaviour;

        std::function<void(BaseBehaviour*)> m_DeleteFunc;
    };
} // namespace Ease

#endif