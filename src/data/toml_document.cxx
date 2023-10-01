#include "toml_document.hxx"

#include <toml.hpp>

#include <iostream>

#include "servers/file_server.hxx"

struct toml_document_data {
	toml::table table;

	static toml_document_data &get(void *internal) {
		return *reinterpret_cast<toml_document_data *>(internal);
	}
};

toml_document::toml_document() {
	m_internal = new toml_document_data;
}

toml_document::toml_document(const toml_document &doc) {
	m_internal = new toml_document_data;
	toml_document_data::get(m_internal).table = toml_document_data::get(doc.m_internal).table;
}

toml_document::toml_document(const char *path) {
	m_internal = new toml_document_data;
	LoadFile(path);
}

toml_document::~toml_document() {
	delete reinterpret_cast<toml_document_data *>(m_internal);
}

ErrorCode toml_document::LoadFile(const char *path) {
	try {
		std::string buf;
		ErrorCode err = FileServer::get().ReadFileString(path, buf);
		if (err != OK) {
			return err;
		}

		toml_document_data::get(m_internal).table = toml::parse(buf);
	} catch (const std::exception &ex) {
		return ERR_FAILED;
	}
	return OK;
}

ErrorCode toml_document::Serialize(std::string &str) {
	std::stringstream ss;
	ss << toml_document_data::get(m_internal).table << std::endl;

	str = ss.str();
	return OK;
}

template <>
toml_document &toml_document::Set(const char *key, toml_document value) {
	toml_document_data::get(m_internal).table.insert_or_assign(key, toml_document_data::get(value.m_internal).table);
	return *this;
}

template <>
int toml_document::Value(const char *key, int fallback) {
	return toml_document_data::get(m_internal).table[key].value_or(fallback);
}

template <>
toml_document &toml_document::Set(const char *key, int value) {
	toml_document_data::get(m_internal).table.insert_or_assign(key, value);
	return *this;
}

template <>
int64_t toml_document::Value(const char *key, int64_t fallback) {
	return toml_document_data::get(m_internal).table[key].value_or(fallback);
}

template <>
toml_document &toml_document::Set(const char *key, int64_t value) {
	toml_document_data::get(m_internal).table.insert_or_assign(key, value);
	return *this;
}

template <>
std::string toml_document::Value(const char *key, std::string fallback) {
	return toml_document_data::get(m_internal).table[key].value_or(fallback);
}

template <>
toml_document &toml_document::Set(const char *key, std::string value) {
	toml_document_data::get(m_internal).table.insert_or_assign(key, value);
	return *this;
}

template <>
float toml_document::Value(const char *key, float fallback) {
	return toml_document_data::get(m_internal).table[key].value_or(fallback);
}

template <>
toml_document &toml_document::Set(const char *key, float value) {
	toml_document_data::get(m_internal).table.insert_or_assign(key, value);
	return *this;
}

template <>
glm::vec2 toml_document::Value(const char *key, glm::vec2 fallback) {
	toml_document doc;
	toml::table *t = toml_document_data::get(m_internal).table[key].as_table();
	if (nullptr != t) {
		toml_document_data::get(doc.m_internal).table = *t;
	}

	fallback.x = doc.Value("x", fallback.x);
	fallback.y = doc.Value("y", fallback.y);

	return fallback;
}

template <>
toml_document &toml_document::Set(const char *key, glm::vec2 value) {
	toml_document doc;
	doc.Set("x", value.x).Set("y", value.y);
	Set(key, doc);
	return *this;
}

template <>
toml_document &toml_document::Set(const char *key, const char *value) {
	toml_document_data::get(m_internal).table.insert_or_assign(key, value);
	return *this;
}

toml_document toml_document::operator[](const char *v) {

	toml_document doc;
	toml::table *t = toml_document_data::get(m_internal).table[v].as_table();
	if (nullptr != t) {
		toml_document_data::get(doc.m_internal).table = *t;
	}

	return doc;
}

toml_document &toml_document::operator=(const toml_document &o) {
	toml_document_data::get(m_internal).table = toml_document_data::get(o.m_internal).table;
	return *this;
}

toml_document &toml_document::operator=(toml_document &&o) {
	toml_document_data::get(m_internal).table = toml_document_data::get(o.m_internal).table;
	return *this;
}