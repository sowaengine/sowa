#include "document.hpp"

#include "toml.hpp"

namespace sowa
{
    SerializeDocument::SerializeDocument(toml::parse_result doc) {
        m_doc = doc;
    }
    SerializeDocument::SerializeDocument(toml::node& doc) {
        toml::table *table = doc.as_table();
        if(nullptr != table)
            m_doc = *table;
    }
    SerializeDocument::~SerializeDocument() {

    }

    size_t SerializeDocument::Size() {
        return m_doc.size();
    }

    SerializeDocument SerializeDocument::Table(const std::string& name) {
        toml::table* table = m_doc[name].as_table();
        if(nullptr != table && table->type() == toml::node_type::table) {
            return SerializeDocument(*table);
        }
        return SerializeDocument();
        
    }
} // namespace sowa
