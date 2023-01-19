#include "project.hpp"

#include "pugixml.hpp"

using namespace pugi;

namespace Sowa {
Project::Project(EngineContext &ctx)
	: _Context(ctx) {
	_Doc = std::make_unique<xml_document>();
}
Project::~Project() {}

Project &Project::Of(EngineContext *context) {
	return *context->GetSingleton<Project>(Sowa::Server::PROJECT);
}

bool Project::Load(const char *path) {
	_ProjectPath = path;
	_ProjectPath = _ProjectPath / ".swproj";

	xml_parse_result result = _Doc->load_file(_ProjectPath.string().c_str());
	if (!result)
		return false;

	xml_node swproj = _Doc->child("swproj");
	if (!swproj) {
		return false;
	}

	xml_attribute swproj_version = swproj.attribute("version");
	if (!swproj_version) {
		return false;
	}

	unsigned version = swproj_version.as_uint(0);
	if (version == 1) {
		return v1LoadProject();
	}

	Debug::Error("Unable to load project {}", _ProjectPath.string());
	return false;
}

bool Project::Save() {
	unsigned version = 1;

	xml_document doc;
	{
		xml_node swproj = doc.append_child("swproj");
		swproj.append_attribute("version").set_value(version);
		{
			xml_node settings = swproj.append_child("settings");
			{
				xml_node application = settings.append_child("application");
				{
					xml_node name = application.append_child("name");
					name.text().set(proj.settings.application.name.c_str());

					xml_node desc = application.append_child("desc");
					desc.text().set(proj.settings.application.desc.c_str());

					xml_node mainscene = application.append_child("mainscene");
					mainscene.text().set(proj.settings.application.mainscene.c_str());

					xml_node icon = application.append_child("icon");
					icon.text().set(proj.settings.application.icon.c_str());
				}

				xml_node window = settings.append_child("window");
				window.append_attribute("fullscreen").set_value(proj.settings.window.fullscreen);
				{
					xml_node windowsize = window.append_child("windowsize");
					windowsize.append_attribute("w").set_value(proj.settings.window.windowsize.x);
					windowsize.append_attribute("h").set_value(proj.settings.window.windowsize.y);

					xml_node videosize = window.append_child("videosize");
					videosize.append_attribute("w").set_value(proj.settings.window.videosize.x);
					videosize.append_attribute("h").set_value(proj.settings.window.videosize.y);
				}
			}
		}
	}

	return doc.save_file(_ProjectPath.string().c_str());
}

bool Project::v1LoadProject() {
	xpath_node settings = _Doc->select_node("/swproj/settings");
	if (settings) {
		xml_node application = settings.node().child("application");
		if (application) {
			proj.settings.application.name = application.child("name").text().as_string(proj.settings.application.name.c_str());
			proj.settings.application.desc = application.child("desc").text().as_string(proj.settings.application.desc.c_str());
			proj.settings.application.mainscene = application.child("mainscene").text().as_string(proj.settings.application.mainscene.c_str());
			proj.settings.application.icon = application.child("icon").text().as_string(proj.settings.application.icon.c_str());
		}

		xml_node window = settings.node().child("window");
		if (window) {
			proj.settings.window.fullscreen = window.attribute("fullscreen").as_bool(proj.settings.window.fullscreen);
			proj.settings.window.windowsize.x = window.child("windowsize").attribute("w").as_float(proj.settings.window.windowsize.x);
			proj.settings.window.windowsize.y = window.child("windowsize").attribute("h").as_float(proj.settings.window.windowsize.y);
			proj.settings.window.videosize.x = window.child("videosize").attribute("w").as_float(proj.settings.window.videosize.x);
			proj.settings.window.videosize.y = window.child("videosize").attribute("h").as_float(proj.settings.window.videosize.y);
		}
	}

	return true;
}
} // namespace Sowa
