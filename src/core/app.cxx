#include "app.hxx"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/projection.hpp"

#include "core/graphics.hxx"
#include "core/time.hxx"

#include "servers/file_server.hxx"
#include "servers/input_server.hxx"
#include "servers/rendering_server.hxx"

#include "data/lrtb_flags.hxx"
#include "data/toml_document.hxx"

#include "ui/new_container.hxx"
#include "ui/new_tree.hxx"

#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef SW_WEB
#include <emscripten.h>
#endif

static App *s_instance;

App::App() {
	s_instance = this;
}

App::~App() {
}

App &App::GetInstance() {
	return *s_instance;
}

#ifdef SW_WEB
EM_JS(void, sync_fs_from_db, (), {
	Module.timer = false;
	FS.syncfs(
		true, function(err) {
			if (err) {
				console.error("An error occured while syncing fs", err);
			}
			Module.timer = true;
		});
});

EM_JS(bool, check_timer, (), {
	return Module.timer;
});
#endif

Error App::Init() {
#ifndef SW_WEB
	m_appPath = "./project";
#else
	m_appPath = "/app";
#endif
	FileServer::Create(this);

	// Create working dir
#ifdef SW_WEB
	EM_ASM({
		let text = Module.UTF8ToString($0, $1);

		FS.mkdir(text);
		FS.mount(IDBFS, {}, text);
	},
		   m_appPath.string().c_str(), m_appPath.string().size());
	sync_fs_from_db();

	while (!check_timer()) {
		emscripten_sleep(1);
	}
#endif

	Error err = m_projectSettings.Load("res://project.sowa");
	if (err != OK) {
		//
	}

	RenderingServer::GetInstance().CreateWindow(m_projectSettings.window_width, m_projectSettings.window_height, m_projectSettings.app_name);

	m_layer2D.SetTarget(0, RenderLayerTargetType::Vec4);
	m_layer2D.SetTarget(1, RenderLayerTargetType::Int);
	m_layer2D.Create(1920, 1080);

	// m_layerUI.SetTarget(0, RenderLayerTargetType::Vec4);
	// m_layerUI.SetTarget(1, RenderLayerTargetType::Int);
	// m_layerUI.Create(1920, 1080);

#ifdef SW_WEB
	// Update page title
	if (m_projectSettings.app_name != "") {
		EM_ASM({
			document.title = Module.UTF8ToString($0, $1);
			console.log(document.title);
		},
			   m_projectSettings.app_name.c_str(), m_projectSettings.app_name.size());
	}
#endif

	// Initialize rendering
	ModelBuilder::Quad2D(rectModel);
	ModelBuilder::Quad2D(fullscreenModel, 2.f);

	err = mainShader.Load("res://shaders/main.vs", "res://shaders/main.fs");
	if (err != OK) {
		std::cerr << "Failed to load main shader" << err << std::endl;
	}

	err = fullscreenShader.Load("res://shaders/fullscreen.vs", "res://shaders/fullscreen.fs");
	if (err != OK) {
		std::cerr << "Failed to load fullscreen shader" << std::endl;
	}

	err = uiShader.Load("res://shaders/ui_panel.vs", "res://shaders/ui_panel.fs");
	if (err != OK) {
		std::cerr << "Failed to load ui shader" << std::endl;
	}

	err = m_testTexture.Load(TextureType::Texture2D, "res://image.png");
	if (err != OK) {
		std::cout << "Failed to load texture: " << err << std::endl;
	}

	err = m_batchRenderer.Init("res://shaders/batch2d.vs", "res://shaders/batch2d.fs");
	if (err != OK) {
		std::cout << "Failed to load renderer: " << err << std::endl;
	}

	Time::update();

	MouseInputCallback().append([this](InputEventMouseButton event) {
		if (event.action == PRESSED) {
			if (this->m_resizeContainerID != 0 && !this->m_resizing) {
				this->m_resizing = true;
			}
		} else if (event.action == RELEASED) {
			if (this->m_resizing) {
				this->m_resizing = false;
				this->m_resizeContainerID = 0;
			}
		}
	});

	MouseMoveCallback().append([this](InputEventMouseMove event) {
		if (this->m_resizing) {
			int w, h;
			RenderingServer::GetInstance().GetWindowSize(w, h);

			event.deltaX *= (1920.f / (float)w);
			event.deltaY *= (1080.f / (float)h);

			NewContainer *resizeContainer = this->m_uiTree.GetContainerByID(this->m_resizeContainerID);
			if (nullptr == resizeContainer)
				return;

			// root container cannot be resized
			NewContainer *parent = resizeContainer->GetParent();
			if (nullptr == parent)
				return;

			if (this->m_resizeFlags.right) {
				float currentWidth = resizeContainer->Width();

				float newSize = resizeContainer->m_sizePercentage * (currentWidth + event.deltaX) / currentWidth;
				float sizeDiff = newSize - resizeContainer->m_sizePercentage;
				int idx = parent->GetChildIndex(this->m_resizeContainerID);
				if (idx == -1 && idx >= parent->ChildCount() - 2)
					return;

				NewContainer *nextChild = parent->Child(idx + 1);
				if (nullptr == nextChild)
					return;

				if (nextChild->m_sizePercentage - sizeDiff < nextChild->minWidth) {
					sizeDiff = nextChild->m_sizePercentage - nextChild->minWidth;
				}
				if (resizeContainer->m_sizePercentage + sizeDiff < resizeContainer->minWidth) {
					sizeDiff = -(resizeContainer->m_sizePercentage - resizeContainer->minWidth);
				}
				if (nextChild->m_sizePercentage - sizeDiff > nextChild->maxWidth) {
					sizeDiff = -(nextChild->maxWidth - nextChild->m_sizePercentage);
				}
				if (resizeContainer->m_sizePercentage + sizeDiff > resizeContainer->maxWidth) {
					sizeDiff = resizeContainer->maxWidth - resizeContainer->m_sizePercentage;
				}

				nextChild->m_sizePercentage -= sizeDiff;
				resizeContainer->m_sizePercentage += sizeDiff;
			} else if (this->m_resizeFlags.left) {
				float currentWidth = resizeContainer->Width();

				float newSize = resizeContainer->m_sizePercentage * (currentWidth + event.deltaX) / currentWidth;
				float sizeDiff = newSize - resizeContainer->m_sizePercentage;
				int idx = parent->GetChildIndex(this->m_resizeContainerID);
				if (idx <= 0)
					return;

				NewContainer *previousChild = parent->Child(idx - 1);
				if (nullptr == previousChild)
					return;

				if (previousChild->m_sizePercentage + sizeDiff < previousChild->minWidth) {
					sizeDiff = -(previousChild->m_sizePercentage - previousChild->minWidth);
				}
				if (resizeContainer->m_sizePercentage - sizeDiff < resizeContainer->minWidth) {
					sizeDiff = resizeContainer->m_sizePercentage - resizeContainer->minWidth;
				}
				if (previousChild->m_sizePercentage + sizeDiff > previousChild->maxWidth) {
					sizeDiff = previousChild->maxWidth - previousChild->m_sizePercentage;
				}
				if (resizeContainer->m_sizePercentage - sizeDiff > resizeContainer->maxWidth) {
					sizeDiff = -(resizeContainer->maxWidth - resizeContainer->m_sizePercentage);
				}

				previousChild->m_sizePercentage += sizeDiff;
				resizeContainer->m_sizePercentage -= sizeDiff;
			} else if (this->m_resizeFlags.bottom) {
				float currentHeight = resizeContainer->Height();

				float newSize = resizeContainer->m_sizePercentage * (currentHeight + event.deltaY) / currentHeight;
				float sizeDiff = newSize - resizeContainer->m_sizePercentage;
				int idx = parent->GetChildIndex(this->m_resizeContainerID);
				if (idx <= 0)
					return;

				NewContainer *previousChild = parent->Child(idx - 1);
				if (nullptr == previousChild)
					return;

				if (previousChild->m_sizePercentage - sizeDiff < previousChild->minWidth) {
					sizeDiff = previousChild->m_sizePercentage - previousChild->minWidth;
				}
				if (resizeContainer->m_sizePercentage + sizeDiff < resizeContainer->minWidth) {
					sizeDiff = -(resizeContainer->m_sizePercentage - resizeContainer->minWidth);
				}
				if (previousChild->m_sizePercentage - sizeDiff > previousChild->maxWidth) {
					sizeDiff = previousChild->maxWidth - previousChild->m_sizePercentage;
				}
				if (resizeContainer->m_sizePercentage + sizeDiff > resizeContainer->maxWidth) {
					sizeDiff = resizeContainer->maxWidth - resizeContainer->m_sizePercentage;
				}

				previousChild->m_sizePercentage -= sizeDiff;
				resizeContainer->m_sizePercentage += sizeDiff;
			} else if (this->m_resizeFlags.top) {
				float currentHeight = resizeContainer->Height();

				float newSize = resizeContainer->m_sizePercentage * (currentHeight + event.deltaY) / currentHeight;
				float sizeDiff = newSize - resizeContainer->m_sizePercentage;
				int idx = parent->GetChildIndex(this->m_resizeContainerID);
				if (idx == -1 && idx >= parent->ChildCount() - 2)
					return;

				NewContainer *nextChild = parent->Child(idx + 1);
				if (nullptr == nextChild)
					return;

				if (nextChild->m_sizePercentage + sizeDiff < nextChild->minWidth) {
					sizeDiff = -(nextChild->m_sizePercentage - nextChild->minWidth);
				}
				if (resizeContainer->m_sizePercentage - sizeDiff < resizeContainer->minWidth) {
					sizeDiff = resizeContainer->m_sizePercentage - resizeContainer->minWidth;
				}
				if (nextChild->m_sizePercentage + sizeDiff > nextChild->maxWidth) {
					sizeDiff = nextChild->maxWidth - nextChild->m_sizePercentage;
				}
				if (resizeContainer->m_sizePercentage - sizeDiff > resizeContainer->maxWidth) {
					sizeDiff = -(resizeContainer->maxWidth - resizeContainer->m_sizePercentage);
				}

				nextChild->m_sizePercentage += sizeDiff;
				resizeContainer->m_sizePercentage -= sizeDiff;
			}
		}
	});

	m_batchRenderer.GetShader().UniformMat4("uProj", glm::ortho(0.f, 1920.f, 0.f, 1080.f));
	m_batchRenderer.GetShader().UniformMat4("uView", glm::mat4(1.f));

	m_uiTree.Root().SetOrientation(ContainerOrientation::Column);
	m_uiTree.Root().SetChildren({3, 93, 4});

	m_uiTree.Root().Child(0)->maxWidth = 3.f;
	m_uiTree.Root().Child(0)->minWidth = 3.f;
	m_uiTree.Root().Child(0)->resizable = false;
	m_uiTree.Root().Child(0)->color = Color::FromRGB(42, 202, 234);

	m_uiTree.Root().Child(2)->maxWidth = 4.f;
	m_uiTree.Root().Child(2)->minWidth = 4.f;
	m_uiTree.Root().Child(2)->resizable = false;

	m_uiTree.Root().Child(1)->SetOrientation(ContainerOrientation::Row);
	m_uiTree.Root().Child(1)->SetChildren({20.f, 55.f, 25.f});

	m_uiTree.Root().Child(1)->Child(1)->SetOrientation(ContainerOrientation::Column);
	m_uiTree.Root().Child(1)->Child(1)->SetChildren({30.f, 70.f});

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	return OK;
}

Error App::Run() {
#ifdef SW_WEB
	emscripten_set_main_loop_arg(mainLoopCaller, this, 0, 1);
#else
	while (!RenderingServer::GetInstance().WindowShouldClose()) {
		mainLoop();
	}
#endif
	return OK;
}

void App::SetRenderLayer(RenderLayer *renderlayer) {
	if (nullptr == renderlayer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		int width, height;
		RenderingServer::GetInstance().GetWindowSize(width, height);
		glViewport(0, 0, width, height);

		return;
	}

	renderlayer->Bind();
}

void App::mainLoop() {
	InputServer::GetInstance().ProcessInput();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	Time::update();

	SetRenderLayer(&m_layer2D);
	m_layer2D.Clear(0.5f, 0.5f, 0.5f, 1.0f);

	int w, h;
	RenderingServer::GetInstance().GetWindowSize(w, h);

	mainShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_testTexture.ID());

	uiShader.Bind();

	CursorMode cursorMode = CursorMode::Normal;

	double x, y;
	InputServer::GetInstance().GetMousePosition(x, y);
	x *= (1920.f / (float)w);
	y *= (1080.f / (float)h);
	m_hoveredItem = m_layer2D.ReadAttachmentInt(1, x, y);

	if (!m_resizing) {
		m_resizeFlags.left = false;
		m_resizeFlags.top = false;
		m_resizeFlags.right = false;
		m_resizeFlags.bottom = false;
		m_resizeContainerID = 0;
	}

	NewContainer *cont = m_uiTree.GetContainerByID(m_hoveredItem);
	if (cont != nullptr) {
		if (!cont->resizable)
			return;

		LRTBFlags flags;

		// if parent is row, last element cannot be resized to right
		// if parent is column, last element cannot be resized to top
		// if parent is row, first element cannot be resized to left
		// if parent is column, first element cannot be resized to bottom

		NewContainer *parent = cont->GetParent();
		if (nullptr != parent) {

			int index = parent->GetChildIndex(cont->ID());
			if (parent->GetOrientation() == ContainerOrientation::Row) {
				flags.right = true;
				flags.left = true;

				if (index == parent->ChildCount() - 1) {
					flags.right = false;
				} else if (index == 0) {
					flags.left = false;
				}
			} else {
				flags.top = true;
				flags.bottom = true;

				if (index == parent->ChildCount() - 1) {
					flags.top = false;
				} else if (index == 0) {
					flags.bottom = false;
				}
			}
		}

		float itemLeft = cont->PosX();
		float itemRight = cont->PosX() + cont->Width();
		float itemBottom = cont->PosY();
		float itemTop = cont->PosY() + cont->Height();
		float resizeWidth = 20.f;

		double x, y;
		InputServer::GetInstance().GetMousePosition(x, y);
		y = h - y;
		x *= (1920.f / (float)w);
		y *= (1080.f / (float)h);

		LRTBFlags resized_on;
		if (flags.left) {
			if (x > itemLeft && x < itemLeft + resizeWidth) {
				resized_on.left = true;
			}
		}

		if (flags.right) {
			if (x > itemRight - resizeWidth && x < itemRight) {
				resized_on.right = true;
			}
		}

		if (flags.top) {
			if (y > itemTop - resizeWidth && y < itemTop) {
				resized_on.top = true;
			}
		}

		if (flags.bottom) {
			if (y > itemBottom && y < itemBottom + resizeWidth) {
				resized_on.bottom = true;
			}
		}

		unsigned int resize = 0b00;
		resize |= (resized_on.left || resized_on.right) << 0;
		resize |= (resized_on.bottom || resized_on.top) << 1;

		if (resize == 0b01) {
			cursorMode = CursorMode::ResizeX;
		} else if (resize == 0b10) {
			cursorMode = CursorMode::ResizeY;
		} else if (resize == 0b11) {
			cursorMode = CursorMode::Resize;
		}

		if (!m_resizing) {
			m_resizeFlags = resized_on;
			m_resizeContainerID = cont->ID();
		}
	}

	Renderer().Reset();

	static float f = 0.f;
	f += 0.1f;

	static int frame = 0;
	frame++;
	// std::cout << "Frame: " << frame << std::endl;
	static std::chrono::time_point t = std::chrono::system_clock::now();
	if (frame == 60) {
		std::chrono::time_point now = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> ms = now - t;

		std::cout << "took " << ms.count() << "ms" << std::endl;
	}

	m_uiTree.Root().DrawLayout(0.f, 0.f, 1920.f, 1080.f);
	float size = 64.f;
	for (float x = 550.f; x < 1200.f; x += size) {
		for (float y = 450.f; y < 850.f; y += size) {

			float sinf = std::sin((x / 64.f + y / 64.f) * f * 0.02f) * 100;
			float cosf = std::cos((x / 64.f + y / 64.f) * f * 0.02f) * 100;
			Renderer().PushQuad(x + sinf, y + cosf, 0.f, size, size, fmod(x * 1.2f, 1.f), fmod(y * 0.2f, 1.f), fmod((x * 1.5f + y * 5.1f), 1.f), 1.f, 1.f, m_testTexture.ID());
		}
	}

	Renderer().End();

	SetRenderLayer(nullptr);

	RenderingServer::GetInstance().SetCursorMode(cursorMode);

	fullscreenShader.Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_layer2D.GetTargetTextureID(0));
	fullscreenModel.Draw();

	RenderingServer::GetInstance().SwapBuffers();
	InputServer::GetInstance().PollEvents();
}

void App::mainLoopCaller(void *self) {
	static_cast<App *>(self)->mainLoop();
}

extern "C" void Unload() {
#ifdef SW_WEB
	EM_ASM(
		FS.syncfs(
			false, function(err) {
				if (err) {
					alert("An error occured while syncing fs", err);
				} else {
					console.log("Successfully synced");
				}
			}););
#endif
}