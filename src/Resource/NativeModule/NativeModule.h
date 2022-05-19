#ifndef NATIVEMODULE_H
#define NATIVEMODULE_H

#pragma once

#include <stdint.h>
#include "Ease.hpp"
#include "Resource/Resource.h"
#include "Ease.hpp"
#include <functional>

typedef uint32_t ResourceID;

namespace Ease
{
    class NativeModule : public BaseResource
    {
    public:
        NativeModule();
        ~NativeModule();
        
        void CallStart();
        void CallUpdate();
        void CallOnImGuiRender();

        inline void SetModule(BaseModule* _module) { m_pModule = _module;}

        void SetDeleteFunc(std::function<void(BaseModule*)> func) { m_DeleteFunc = func; }
    private:
        friend class Application;
        BaseModule* m_pModule;

        std::function<void(BaseModule*)> m_DeleteFunc;
    };
} // namespace Ease

#endif