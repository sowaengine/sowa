#ifndef SW_EDITOR_CONFIG_HXX
#define SW_EDITOR_CONFIG_HXX

#include <string>

#include "core/error/error.hxx"
#include "data/toml_document.hxx"

struct EditorConfig {
	static EditorConfig &get();

	ErrorCode Load();
	ErrorCode Save();

	std::string last_project = "";
	bool auto_open_last_project = false;

  private:
	toml_document m_doc;

	ErrorCode loadVersion1(toml_document &doc);
};

#endif // SW_EDITOR_CONFIG_HXX