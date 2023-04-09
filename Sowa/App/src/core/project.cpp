#include "project.hpp"
#include "serialize/serializer.hpp"
#include "utils/file.hpp"

namespace Sowa {
Project::Project(EngineContext &ctx)
	: _Context(ctx) {
		m_Type = Typename();
}
Project::~Project() {}

Project &Project::Of(EngineContext *context) {
	return *context->GetSingleton<Project>(Sowa::Server::PROJECT);
}

bool Project::Load(const char *path) {
	_ProjectPath = path;
	if(_ProjectPath.filename() != "project.sowa")
		_ProjectPath = _ProjectPath / "project.sowa";

	try {
		m_Doc = File::LoadFile(_ProjectPath);
	}
	catch(const std::exception& e) {
		return false;
	}
	
	if(!Serializer::get_singleton().Load(this, m_Doc)) {
		Debug::Error("Unable to load project {}", _ProjectPath.string());
		return false;
	}
	return true;
}

bool Project::Save() {
	YamlNode project = Serializer::get_singleton().Save(this).Yaml();

	YAML::Emitter emitter;
	emitter << project;
	std::ofstream ofstream(_ProjectPath.string());
	ofstream << emitter.c_str();
	ofstream.close();
	return true;
}



FileBuffer Project::SaveImpl(ObjectType* out) {
	YamlNode doc;
	Project* o = reinterpret_cast<Project*>(out);

	YamlNode application = YamlNode();
	application["Name"] = o->proj.settings.application.name;
	application["Desc"] = o->proj.settings.application.desc;
	application["MainScene"] = o->proj.settings.application.mainscene;
	application["Icon"] = o->proj.settings.application.icon;

	YamlNode window = YamlNode();
	window["FullScreen"] = o->proj.settings.window.fullscreen;
	window["WindowSize"] = Size::SaveImpl(&o->proj.settings.window.windowsize).Yaml();
	window["VideoSize"] = Size::SaveImpl(&o->proj.settings.window.videosize).Yaml();

	YamlNode settings = YamlNode();
	settings["Application"] = application;
	settings["Window"] = window;

	doc["Project"] = YamlNode();
	doc["Project"]["Settings"] = settings;

	return FileBuffer(doc);
}

bool Project::LoadImpl(ObjectType* out, const FileBuffer& buf) {
	YAML::Node doc = buf.Yaml();

	Project* obj = reinterpret_cast<Project*>(out);
	YamlNode project = doc["Project"];
	if(!project) {
		Debug::Error("Project file missing project attribute");
		return false;
	}

	YamlNode settings = project["Settings"];
	if(settings) {
		YamlNode application = settings["Application"];
		if(application) {
			SERIALIZE_GETATTR(std::string, obj->proj.settings.application.name, application["Name"]);
			SERIALIZE_GETATTR(std::string, obj->proj.settings.application.desc, application["Desc"]);
			SERIALIZE_GETATTR(std::string, obj->proj.settings.application.mainscene, application["MainScene"]);
			SERIALIZE_GETATTR(std::string, obj->proj.settings.application.icon, application["Icon"]);
		}
		YamlNode window = settings["Window"];
		if(window) {
			SERIALIZE_GETATTR(bool, obj->proj.settings.window.fullscreen, window["FullScreen"]);
			Size::LoadImpl(&obj->proj.settings.window.windowsize, FileBuffer(window["WindowSize"]));
			Size::LoadImpl(&obj->proj.settings.window.videosize, FileBuffer(window["VideoSize"]));
		}
	}

	return true;
}


} // namespace Sowa
