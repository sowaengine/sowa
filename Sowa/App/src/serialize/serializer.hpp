#ifndef SW_SERIALIZER_HPP_
#define SW_SERIALIZER_HPP_

#include "yaml-cpp/yaml.h"

#define SERIALIZE_GETATTR(t, attr, node) attr = node.as<t>(attr)

namespace Sowa
{
    typedef YAML::Node YamlNode;

    template<typename T>
    struct Serializer {
        YamlNode Save(const T&);
        bool Load(T&, const YamlNode&);
    };
} // namespace Sowa


#endif // SW_SERIALIZER_HPP_