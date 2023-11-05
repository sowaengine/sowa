#include "app.hxx"

#include "core/error.hxx"

#include "res/shaders/batch2d.fs.res.h"
#include "res/shaders/batch2d.vs.res.h"
#include "res/shaders/fullscreen.fs.res.h"
#include "res/shaders/fullscreen.vs.res.h"

void App::load_resources() {
	ErrorCode err;
	fullscreenShader.SetVertexSource(std::string((char *)res::src_res_shaders_fullscreen_vs_res_h_data, sizeof(res::src_res_shaders_fullscreen_vs_res_h_data)));
	fullscreenShader.SetFragmentSource(std::string((char *)res::src_res_shaders_fullscreen_fs_res_h_data, sizeof(res::src_res_shaders_fullscreen_fs_res_h_data)));
	err = fullscreenShader.Build();
	if (err != OK) {
		Utils::Error("Failed to load fullscreen shader");
	}

	err = m_batchRenderer.Init(res::src_res_shaders_batch2d_vs_res_h_data, sizeof(res::src_res_shaders_batch2d_vs_res_h_data), res::src_res_shaders_batch2d_fs_res_h_data, sizeof(res::src_res_shaders_batch2d_fs_res_h_data));
	if (err != OK) {
		Utils::Error("Failed to load renderer: {}", (int)err);
	}

	err = m_testFont.LoadTTF("res://Roboto-Medium.ttf");
	if (err != OK) {
		Utils::Error("Failed to load font: {}", (int)err);
	}
}