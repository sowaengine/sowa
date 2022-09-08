#ifndef _E_NATIVEMODULE_HPP__
#define _E_NATIVEMODULE_HPP__

#pragma once

#include <stdint.h>
#include "Ease.hpp"
#include "Resource/Resource.hpp"
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
        inline BaseModule* GetModule() { return m_pModule; }

        void SetDeleteFunc(std::function<void(BaseModule*)> func) { m_DeleteFunc = func; }

        // Adds dynamic library extension based on operating system (.dll, .so, .dylib)
        static std::string AddExtension(const std::string& base);
    private:
        friend class Application;
        BaseModule* m_pModule;

        std::function<void(BaseModule*)> m_DeleteFunc;
    };
} // namespace Ease

#endif