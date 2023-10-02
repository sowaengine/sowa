#include "sprite_sheet_animation.hxx"

#include "servers/file_server.hxx"

#include "yaml-cpp/yaml.h"

SpriteSheetAnimation::~SpriteSheetAnimation() {
}

ErrorCode SpriteSheetAnimation::Load(const char *path) {
	std::string data;
	if (ErrorCode err = FileServer::get().ReadFileString(path, data); err != OK) {
		return err;
	}

	YAML::Node yaml = YAML::Load(data.c_str());
	if (yaml["type"].as<std::string>("") != "Animation")
		return ERR_INVALID_FILE;

	YAML::Node textures = yaml["textures"];
	if (!textures) {
	}

	for (auto tex : textures) {
		std::string path = tex["path"].as<std::string>("");
		if (path != "") {
			AnimationTexture &texture = m_textures.emplace_back();
			ErrorCode err = texture.texture.Load(TextureType::Texture2D, path.c_str());
			if (err != OK) {
				Utils::Error("Invalid texture on SpriteSheetAnimation {}, err code: {}", path.c_str(), (int)err);
				return err;
			}
			texture.col = tex["col"].as<uint32_t>(0);
			texture.row = tex["row"].as<uint32_t>(0);
		}
	}

	YAML::Node animations = yaml["animations"];
	if (!animations) {
	}

	for (auto anim : animations) {
		std::string name = anim["name"].as<std::string>("");
		if (name != "") {
			Animation &animation = m_animations.emplace_back();
			animation.name = name;

			YAML::Node tiles = anim["tiles"];
			for (auto tile : tiles) {
				if (!tile.IsSequence() || tile.size() != 3) {
					Utils::Error("Invalid animation tile on SpriteSheetAnimation {}", path);
					continue;
				}

				AnimationTile &t = animation.tiles.emplace_back();
				t.texture_index = tile[0].as<uint32_t>();
				t.row_index = tile[1].as<uint32_t>();
				t.col_index = tile[2].as<uint32_t>();
			}
		}
	}

	return OK;
}
