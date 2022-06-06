#ifndef NATIVEBEHAVIOUR_H
#define NATIVEBEHAVIOUR_H

#pragma once

#include <stdint.h>
#include "Ease.hpp"
#include "Resource/Resource.h"
#include "Ease.hpp"
#include <functional>
#include <vector>

typedef uint32_t ResourceID;

namespace Ease
{
    class NativeBehaviour : public BaseResource
    {
    public:
        NativeBehaviour();
        ~NativeBehaviour();
        
        Ease::BaseBehaviour* Create();
        void Delete();


        inline std::vector<Ease::BaseBehaviour*>& GetBehaviour() { return m_Behaviours; }

        void SetCreateFunc(std::function<BaseBehaviour*()> func) { m_CreateFunc = func; }
        void SetDeleteFunc(std::function<void(BaseBehaviour*)> func) { m_DeleteFunc = func; }
        
        const std::string& GetFilepath() { return m_Filepath; }
        std::string m_Filepath{""};
    private:
        friend class Application;

        std::function<BaseBehaviour*()> m_CreateFunc;
        std::function<void(BaseBehaviour*)> m_DeleteFunc;
        
        std::vector<Ease::BaseBehaviour*> m_Behaviours;
    };
} // namespace Ease

#endif