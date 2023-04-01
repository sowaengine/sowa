#include "project.hpp"
#include "serialize/serializer.hpp"

namespace Sowa {
Project::Project(EngineContext &ctx)
	: _Context(ctx) {
}
Project::~Project() {}

Project &Project::Of(EngineContext *context) {
	return *context->GetSingleton<Project>(Sowa::Server::PROJECT);
}

bool Project::Load(const char *path) {
	_ProjectPath = path;
	_ProjectPath = _ProjectPath / "project.sowa";

	m_Doc = YAML::LoadFile(_ProjectPath.string());
	if(!Serializer<Project>().Load(*this, m_Doc)) {
		Debug::Error("Unable to load project {}", _ProjectPath.string());
		return false;
	}
	return true;
}

bool Project::Save() {
	YamlNode project = Serializer<Project>().Save(*this);

	YAML::Emitter emitter;
	emitter << project;
	std::ofstream ofstream(_ProjectPath.string());
	ofstream << emitter.c_str();
	ofstream.close();
	return true;
}



template<>
YamlNode Serializer<Project>::Save(const Project& in) {
	YamlNode doc;

	YamlNode application = YamlNode();
	application["Name"] = in.proj.settings.application.name;
	application["Desc"] = in.proj.settings.application.desc;
	application["MainScene"] = in.proj.settings.application.mainscene;
	application["Icon"] = in.proj.settings.application.icon;

	YamlNode window = YamlNode();
	window["FullScreen"] = in.proj.settings.window.fullscreen;
	window["WindowSize"] = Serializer<Size>().Save(in.proj.settings.window.windowsize);
	window["VideoSize"] = Serializer<Size>().Save(in.proj.settings.window.videosize);

	YamlNode settings = YamlNode();
	settings["Application"] = application;
	settings["Window"] = window;

	doc["Project"] = YamlNode();
	doc["Project"]["Settings"] = settings;
	return doc;
}

template<>
bool Serializer<Project>::Load(Project& out, const YamlNode& doc) {
	YamlNode project = doc["Project"];
	if(!project) {
		Debug::Error("Project file missing project attribute");
		return false;
	}

	YamlNode settings = project["Settings"];
	if(settings) {
		YamlNode application = settings["Application"];
		if(application) {
			SERIALIZE_GETATTR(std::string, out.proj.settings.application.name, application["Name"]);
			SERIALIZE_GETATTR(std::string, out.proj.settings.application.desc, application["Desc"]);
			SERIALIZE_GETATTR(std::string, out.proj.settings.application.mainscene, application["MainScene"]);
			SERIALIZE_GETATTR(std::string, out.proj.settings.application.icon, application["Icon"]);
		}
		YamlNode window = settings["Window"];
		if(window) {
			SERIALIZE_GETATTR(bool, out.proj.settings.window.fullscreen, window["FullScreen"]);
			Serializer<Size>().Load(out.proj.settings.window.windowsize, window["WindowSize"]);
			Serializer<Size>().Load(out.proj.settings.window.videosize, window["VideoSize"]);
		}
	}

	return true;
}


} // namespace Sowa
