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

   YAML::Emitter& operator<<(YAML::Emitter& out, const b2Vec2& vec)
   {
      out << YAML::Flow;
      out << YAML::BeginSeq << vec.x << vec.y << YAML::EndSeq;
      return out;
   }

   YAML::Emitter& operator<<(YAML::Emitter& out, const Ease::PhysicsBodyType& type)
   {
      if(type == Ease::PhysicsBodyType::STATIC)
         out << "STATIC";
      else if(type == Ease::PhysicsBodyType::DYNAMIC)
         out << "DYNAMIC";
      else if(type == Ease::PhysicsBodyType::KINEMATIC)
         out << "KINEMATIC";
      else
         out << "STATIC";
      return out;
   }

   YAML::Emitter& operator<<(YAML::Emitter& out, const Ease::ColliderShape2D& type)
   {
      if(type == Ease::ColliderShape2D::BOX)
         out << "BOX";
      else if(type == Ease::ColliderShape2D::CIRCLE)
         out << "CIRCLE";
      else
         out << "BOX";
      return out;
   }
} // namespace YAML

namespace Ease::Utils
{
   
} // namespace Ease::Utils
