#include "YAML.hpp"

namespace YAML
{
   YAML::Emitter& operator<<(YAML::Emitter& out, const ImVec2& vec)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
      return out;
   }
   YAML::Emitter& operator<<(YAML::Emitter& out, const ImVec4& vec)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
      return out;
   }

   YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& vec)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
      return out;
   }
   YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vec)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
      return out;
   }
   YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& vec)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
      return out;
   }
} // namespace YAML

namespace Ease::Utils
{
   
} // namespace Ease::Utils
