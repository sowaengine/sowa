#ifndef SW_SERIALIZER_HPP_
#define SW_SERIALIZER_HPP_

#include "stlpch.hpp"
#include "yaml-cpp/yaml.h"
#include "object_type.hpp"
#include "core/file_buffer.hpp"

#define SERIALIZE_GETATTR(t, attr, node) attr = node.as<t>(attr)

namespace sowa {
typedef YAML::Node YamlNode;

class Serializer;

// returns yml, png, project.sowa file
typedef std::function<FileBuffer(object_type *)> SaveFunc;

// expects resource file content
typedef std::function<bool(object_type *, const FileBuffer &)> LoadFunc;

struct SerializeImpl {
  public:
    SerializeImpl() = default;

	SerializeImpl(SaveFunc saveFunc, LoadFunc loadFunc);
	~SerializeImpl();

  private:
	friend class Serializer;
	SaveFunc save;
	LoadFunc load;
};

class Serializer {
  public:
	static Serializer &get_singleton() {
		static Serializer s;
		return s;
	}
    void RegisterSerializer(std::string type, SerializeImpl impl);

	template<typename T>
	FileBuffer SaveWithType(object_type* o) {
		return SaveWithTypename(T::Typename(), o);
	}
	FileBuffer SaveWithTypename(std::string type, object_type*);

	FileBuffer Save(object_type *);
	bool Load(object_type *, const FileBuffer &);

  private:
	std::unordered_map<std::string, SerializeImpl> m_Impls;
};
} // namespace sowa

#endif // SW_SERIALIZER_HPP_