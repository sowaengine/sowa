#include "serializer.hpp"
#include "object_type.hpp"
#include "stlpch.hpp"
#include "debug.hpp"

namespace Sowa
{
    SerializeImpl::SerializeImpl(SaveFunc saveFunc, LoadFunc loadFunc) {
        save = saveFunc;
        load = loadFunc;
    }
	SerializeImpl::~SerializeImpl() {

    }

    void Serializer::RegisterSerializer(std::string type, SerializeImpl impl) {
        m_Impls[type] = impl;
    }

    FileBuffer Serializer::Save(ObjectType* obj) {
        if(obj == nullptr) {
            return FileBuffer();
        }

        auto it = m_Impls.find(obj->GetType());
        if(it == m_Impls.end()) {
            Debug::Error("Failed to save: Object type '{}' serializer doesnt exist", obj->GetType());
        }
        if((*it).second.save == nullptr) {
            Debug::Error("Failed to save: Object type '{}' Save function doesnt exist", obj->GetType());
        }

        FileBuffer out = (*it).second.save(obj);
        return out;
    }
    bool Serializer::Load(ObjectType* obj, const FileBuffer& in) {
        if(obj == nullptr) {
            return false;
        }
        auto it = m_Impls.find(obj->GetType());
        if(it == m_Impls.end()) {
            Debug::Error("Failed to load: Object type '{}' serializer doesnt exist", obj->GetType());
        }
        if((*it).second.save == nullptr) {
            Debug::Error("Failed to load: Object type '{}' Load function doesnt exist", obj->GetType());
        }

        bool success = (*it).second.load(obj, in);
        return success;
    }
} // namespace Sowa
