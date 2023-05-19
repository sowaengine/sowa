#ifndef SW_DOCUMENT_HPP_
#define SW_DOCUMENT_HPP_

#include "toml.hpp"
#include <memory>
#include <string>

namespace sowa {
class TomlSerializer;

class SerializeDocument {
  public:
    toml::table_iterator begin() {
        return m_doc.begin();
    }
    toml::table_iterator end() {
        return m_doc.end();
    }

	template <typename T>
	SerializeDocument &Value(const std::string &name, T &value) {
		value = m_doc[name].value_or(value);
		return *this;
	}
	template <typename T>
	SerializeDocument &SelfValue(T &value) {
		value = m_doc.value_or(value);
		return *this;
	}

	SerializeDocument Table(const std::string &name);
	SerializeDocument operator[](size_t i);

	size_t Size();

  public:
	friend class TomlSerializer;

	SerializeDocument(toml::parse_result);
    SerializeDocument(toml::node&);
	SerializeDocument() = default;
	~SerializeDocument();

	toml::parse_result m_doc;
};

} // namespace sowa

#endif // SW_DOCUMENT_HPP_