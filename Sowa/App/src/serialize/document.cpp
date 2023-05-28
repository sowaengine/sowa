#include "document.hpp"

#include "toml.hpp"
#include <sstream>

namespace sowa {
SerializeDocument::SerializeDocument(toml::table doc) {
	m_doc = doc;
}
SerializeDocument::SerializeDocument(toml::node &doc) {
	toml::table *table = doc.as_table();
	if (nullptr != table)
		m_doc = *table;
}
SerializeDocument::~SerializeDocument() {
}

size_t SerializeDocument::Size() {
	return m_doc.size();
}

SerializeDocument SerializeDocument::Table(const std::string &name) {
	toml::table *table = m_doc[name].as_table();
	if (nullptr != table && table->type() == toml::node_type::table) {
		return SerializeDocument(*table);
	}
	return SerializeDocument();
}

std::string SerializeDocument::Serialize() {
	std::stringstream ss;
	ss << m_doc;
	return ss.str();
}

template <>
SerializeDocument &SerializeDocument::Set(const std::string &name, const SerializeDocument &value) {
	m_doc.insert_or_assign(name, value.m_doc);
	return *this;
}
} // namespace sowa
