#ifndef SW_TOML_DOCUMENT_HXX
#define SW_TOML_DOCUMENT_HXX
#pragma once

#include <string>

#include "core/error/error.hxx"

struct toml_document {
  public:
	toml_document();
	toml_document(const toml_document &);
	toml_document(const char *path);
	~toml_document();

	Error LoadFile(const char *path);
	Error Serialize(std::string &str);

	template <typename T>
	T Value(const char *key, T fallback);

	template <typename T>
	toml_document &Set(const char *key, T value);

	toml_document operator[](const char *v);
	toml_document &operator=(const toml_document &o);
	toml_document &operator=(toml_document &&);

  private:
	void *m_internal = nullptr;
};

#endif // SW_TOML_DOCUMENT_HXX