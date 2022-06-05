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
        
        void Create();
        void Delete();

        void CallStart();
        void CallUpdate();

        inline void SetBehaviour(BaseBehaviour* behaviour) { m_pBehaviour = behaviour;}
        inline BaseBehaviour* GetBehaviour() { return m_pBehaviour; }

        void SetCreateFunc(std::function<BaseBehaviour*()> func) { m_CreateFunc = func; }
        void SetDeleteFunc(std::function<void(BaseBehaviour*)> func) { m_DeleteFunc = func; }
        
        const std::string& GetFilepath() { return m_Filepath; }
        std::string m_Filepath{""};
    private:
        friend class Application;
        BaseBehaviour* m_pBehaviour;

        std::function<BaseBehaviour*()> m_CreateFunc;
        std::function<void(BaseBehaviour*)> m_DeleteFunc;
    };
} // namespace Ease

#endif