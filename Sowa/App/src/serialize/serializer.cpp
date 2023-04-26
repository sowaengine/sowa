#include "serializer.hpp"
#include "object_type.hpp"
#include "stlpch.hpp"
#include "debug.hpp"

namespace sowa
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

    FileBuffer Serializer::SaveWithTypename(std::string type, object_type* obj) {
        if(obj == nullptr) {
            return FileBuffer();
        }

        auto it = m_Impls.find(type);
        if(it == m_Impls.end()) {
            Debug::Error("Failed to save: Object type '{}' serializer doesnt exist", obj->get_type());
            return FileBuffer();
        }
        if((*it).second.save == nullptr) {
            Debug::Error("Failed to save: Object type '{}' Save function doesnt exist", obj->get_type());
            return FileBuffer();
        }

        FileBuffer out = (*it).second.save(obj);
        return out;
    }

    FileBuffer Serializer::Save(object_type* obj) {
        if(obj == nullptr) {
            return FileBuffer();
        }

        return SaveWithTypename(obj->get_type(), obj);
    }
    bool Serializer::Load(object_type* obj, const FileBuffer& in) {
        if(obj == nullptr) {
            return false;
        }
        auto it = m_Impls.find(obj->get_type());
        if(it == m_Impls.end()) {
            Debug::Error("Failed to load: Object type '{}' serializer doesnt exist", obj->get_type());
        }
        if((*it).second.save == nullptr) {
            Debug::Error("Failed to load: Object type '{}' Load function doesnt exist", obj->get_type());
        }

        bool success = (*it).second.load(obj, in);
        return success;
    }
} // namespace sowa
