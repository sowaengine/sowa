#include "app.hxx"
#include "scene/sprite_2d.hxx"
#include "servers/rendering_server.hxx"

void App::editor_mouse_move_event(InputEventMouseMove event) {
	int w, h;
	RenderingServer::get().GetWindowSize(w, h);
	float deltaX = event.deltaX * (1920.f / w);
	float deltaY = event.deltaY * (1080.f / h);

	if (Input::IsButtonDown(MB_RIGHT) && Input::IsCursorInside()) {
		this->m_editorCameraPos2d.x -= deltaX * this->m_editorCameraZoom2d;
		this->m_editorCameraPos2d.y += deltaY * this->m_editorCameraZoom2d;
	}

	if (this->m_selectedNode != 0 && GetCurrentScene() && Input::IsButtonUp(MB_RIGHT)) {
		if (this->m_editorState == EditorState::Dragging) {
			Sprite2D *selected = dynamic_cast<Sprite2D *>(GetCurrentScene()->get_node_by_id(this->m_selectedNode));
			if (selected) {
				vec2 offset = vec2(deltaX * this->m_editorCameraZoom2d, -deltaY * this->m_editorCameraZoom2d);
				if (Input::IsKeyDown(KEY_LEFT_SHIFT)) {
					offset *= 0.2f;
				}

				if (!((int)m_actionAxis & (int)EditorActionAxis::X)) {
					offset.x *= 0.f;
				}
				if (!((int)m_actionAxis & (int)EditorActionAxis::Y)) {
					offset.y *= 0.f;
				}

				selected->position() += offset;
				m_actionDeltaVec2 += offset;
			}
		}
	}
}

void App::editor_key_event(InputEventKey event) {
	if (event.action == KEY_PRESSED && event.key == KEY_G && this->m_editorState != EditorState::None) {
		// todo: rollback action
		m_editorState = EditorState::None;
		RenderingServer::get().SetCursorMode(CursorMode::Normal);
	}

	if (event.action == KEY_PRESSED && event.key == KEY_G && this->m_editorState == EditorState::None) {
		m_editorState = EditorState::Dragging;
		RenderingServer::get().SetCursorMode(CursorMode::Tiled);
		m_actionDeltaVec2 = vec2(0.f);
		m_actionAxis = EditorActionAxis::None;
	}

	if (event.action == KEY_PRESSED && event.key == KEY_X) {
		if (m_actionAxis == EditorActionAxis::X) {
			m_actionAxis = EditorActionAxis::None;
		} else {
			m_actionAxis = EditorActionAxis::X;
		}
	}
	if (event.action == KEY_PRESSED && event.key == KEY_Y) {
		if (m_actionAxis == EditorActionAxis::Y) {
			m_actionAxis = EditorActionAxis::None;
		} else {
			m_actionAxis = EditorActionAxis::Y;
		}
	}
}

void App::editor_scroll_event(InputEventScroll event) {
	int w, h;
	RenderingServer::get().GetWindowSize(w, h);

	float oldZoom = this->m_editorCameraZoom2d;

	this->m_editorCameraZoom2d -= (0.05f * event.yOffset);
	this->m_editorCameraZoom2d = std::max(this->m_editorCameraZoom2d, 0.1f);

	vec2 midPoint(1920.f * 0.5f, 1080.f * 0.5f);
	double x, y;
	Input::GetWindowMousePosition(x, y);
	x *= (1920.f / (float)w);
	y *= (1080.f / (float)h);

	if (this->m_editorState != EditorState::None && GetCurrentScene()) {
		if (Sprite2D *sprite = dynamic_cast<Sprite2D *>(GetCurrentScene()->get_node_by_id(this->m_selectedNode)); nullptr != sprite) {
			x = (1920.f * 0.5f);
			y = (1080.f * 0.5f);
		}
	}

	this->m_editorCameraPos2d.x += (x - midPoint.x) * (oldZoom - this->m_editorCameraZoom2d);
	this->m_editorCameraPos2d.y -= (y - midPoint.y) * (oldZoom - this->m_editorCameraZoom2d);
}