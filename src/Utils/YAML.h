#ifndef YAML_H
#define YAML_H

#pragma once

#include "imgui-docking/imgui.h"
#include "yaml-cpp/yaml.h"

namespace YAML
{
    template<>
    struct convert<ImVec2>
    {
        static Node encode(const ImVec2& vec)
        {
            Node node;
            node.push_back(vec.x);
            node.push_back(vec.y);
            return node;
        }
        static bool decode(const Node& node, ImVec2& vec)
        {
            if(!node.IsSequence() || node.size() != 2)
                return false;
            
            vec.x = node[0].as<float>();
            vec.y = node[1].as<float>();
            return true;
        }
    };

    template<>
    struct convert<ImVec4>
    {
        static Node encode(const ImVec4& vec)
        {
            Node node;
            node.push_back(vec.x);
            node.push_back(vec.y);
            node.push_back(vec.z);
            node.push_back(vec.w);
            return node;
        }
        static bool decode(const Node& node, ImVec4& vec)
        {
            if(!node.IsSequence() || node.size() != 4)
                return false;
            
            vec.x = node[0].as<float>();
            vec.y = node[1].as<float>();
            vec.z = node[2].as<float>();
            vec.w = node[3].as<float>();
            return true;
        }
    };
    

    YAML::Emitter& operator<<(YAML::Emitter& out, const ImVec2& vec);
    YAML::Emitter& operator<<(YAML::Emitter& out, const ImVec4& vec);
} // namespace YAML


namespace Ease::Utils
{
    
} // namespace Ease::Utils


#endif