#include "core/window.hpp"
#include "ecs/components/camera2d.hpp"
#include "ecs/world.hpp"
#include "editor/editor.hpp"
#include "editor/icons.hpp"
#include "editor/inspector.hpp"
#include "editor/style.hpp"
#include "editor/viewport.hpp"
#include "imgui.h"
#include "physics/raycast2d.hpp"
#include "resource/resource_registry.hpp"
#include "scene/scene_manager.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace sowa {
namespace {

void drawViewportOutline(ImDrawList &draw, const glm::mat4 &editorViewProjection, const Rect &image,
	const std::array<Vector2, 4> &worldCorners, ImU32 color, float thickness) {
	std::array<ImVec2, 4> points;
	for (size_t i = 0; i < points.size(); ++i) {
		const Vector2 screen =
			editor::WorldToViewportImage(editorViewProjection, worldCorners[i], image);
		if (!std::isfinite(screen.x) || !std::isfinite(screen.y)) {
			return;
		}
		points[i] = ImVec2(screen.x, screen.y);
	}
	draw.AddPolyline(points.data(), static_cast<int>(points.size()), color, ImDrawFlags_Closed,
		thickness);
}

void drawViewportLine(ImDrawList &draw, const glm::mat4 &editorViewProjection, const Rect &image,
	const editor::GizmoLine &line, ImU32 color, float thickness) {
	const Vector2 a = editor::WorldToViewportImage(editorViewProjection, line.a, image);
	const Vector2 b = editor::WorldToViewportImage(editorViewProjection, line.b, image);
	if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(b.x) || !std::isfinite(b.y)) {
		return;
	}
	draw.AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), color, thickness);
}

void drawSelectionMarker(ImDrawList &draw, const glm::mat4 &editorViewProjection, const Rect &image,
	Vector2 worldPosition) {
	const Vector2 projected =
		editor::WorldToViewportImage(editorViewProjection, worldPosition, image);
	if (!std::isfinite(projected.x) || !std::isfinite(projected.y)) {
		return;
	}

	const ImVec2 centre(projected.x, projected.y);
	constexpr float inner = 7.f;
	constexpr float outer = 14.f;
	const ImU32 shadow = IM_COL32(15, 15, 17, 220);
	const ImU32 accent = IM_COL32(165, 155, 255, 255);
	const auto drawCross = [&](ImU32 color, float thickness) {
		draw.AddLine(ImVec2(centre.x - outer, centre.y), ImVec2(centre.x - inner, centre.y), color,
			thickness);
		draw.AddLine(ImVec2(centre.x + inner, centre.y), ImVec2(centre.x + outer, centre.y), color,
			thickness);
		draw.AddLine(ImVec2(centre.x, centre.y - outer), ImVec2(centre.x, centre.y - inner), color,
			thickness);
		draw.AddLine(ImVec2(centre.x, centre.y + inner), ImVec2(centre.x, centre.y + outer), color,
			thickness);
	};
	draw.AddCircle(centre, shadow, 5.5f, 20, 4.f);
	drawCross(shadow, 4.f);
	draw.AddCircle(centre, accent, 5.5f, 20, 2.f);
	drawCross(accent, 2.f);
}

} // namespace

void Editor::drawViewport(u64 gameTextureId, Vector2 gameSize, bool playing, World &world,
	SceneManager &scenes, ResourceRegistry &resources, Entity hoveredEntity) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::Begin(ICON_FA_CUBE "  Viewport");
	ImGui::PopStyleVar();
	drawSceneTabs(scenes, resources, playing);
	handleSelectionShortcuts(resources,
		editor::CanCreateChild(Selected()) ? Selected() : scenes.EditorCurrent().Root());
	if (_gizmoDrag && _gizmoScene.Root() != scenes.EditorCurrent().Root()) {
		finishTransformGizmo();
	}
	const bool hasCamera = scenes.Current().ActiveCamera().IsAlive();
	// Match the image rendered before this frame's UI changes the toggle.
	const bool cameraPreview = _previewSceneCamera && hasCamera;
	if (!playing) {
		ImGui::Indent(10.f);
		ImGui::BeginDisabled(!hasCamera);
		ImGui::Checkbox("Camera Preview", &_previewSceneCamera);
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip(hasCamera ? "Look through the active scene camera."
										: "Set an active camera in the scene Inspector first.");
		}
		ImGui::SameLine(0.f, 18.f);
		drawGizmoToolbar(cameraPreview || (_previewSceneCamera && hasCamera));
		ImGui::Unindent(10.f);
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild("GameImage", ImVec2(0.f, 0.f), ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	_gameImageRect = {};
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	if (avail.x >= 1.f && avail.y >= 1.f && gameTextureId != 0) {
		const Rect fit = editor::FitGameImage(gameSize, Vector2(avail.x, avail.y));
		if (fit.w >= 1.f && fit.h >= 1.f) {
			const glm::mat4 editorViewProjection = editor::ViewProjection(_camera, gameSize);
			ImGui::SetCursorPos(ImVec2(fit.x, fit.y));
			const ImTextureRef tex(static_cast<ImTextureID>(gameTextureId));
			// GL framebuffer color is Y-up; ImGui UVs are Y-down.
			ImGui::Image(tex, ImVec2(fit.w, fit.h), ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			const ImVec2 origin = ImGui::GetMainViewport()->Pos;
			_gameImageRect = Rect(min.x - origin.x, min.y - origin.y, max.x - min.x, max.y - min.y);
			// While the debugger holds a frame, its banner replaces the badge.
			if (scenes.IsPaused() && !scenes.IsScriptSuspended()) {
				const char *badge = ICON_FA_PAUSE "  Paused";
				const ImVec2 textSize = ImGui::CalcTextSize(badge);
				const ImVec2 badgeMin(min.x + 8.f, min.y + 8.f);
				const ImVec2 badgeMax(badgeMin.x + textSize.x + 16.f,
					badgeMin.y + textSize.y + 8.f);
				ImDrawList *badgeDraw = ImGui::GetWindowDrawList();
				badgeDraw->AddRectFilled(badgeMin, badgeMax,
					ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.6f)), 4.f);
				badgeDraw->AddText(ImVec2(badgeMin.x + 8.f, badgeMin.y + 4.f),
					ImGui::GetColorU32(ImGuiCol_Text), badge);
			}

			if (playing || cameraPreview) {
				_marquee.reset();
				_boxRequest.reset();
				finishUIGizmo();
				_cameraDragging = false;
			} else {
				const Rect image(min.x, min.y, max.x - min.x, max.y - min.y);
				ImDrawList *draw = ImGui::GetWindowDrawList();
				draw->PushClipRect(min, max, true);

				const Scene current = scenes.Current();
				const Entity activeCamera = current.ActiveCamera();
				world.Each<Camera2D>([&](Entity camera, Camera2D &) {
					if (!current.IsValid() || !editor::IsInSceneTree(camera, current.Root())) {
						return;
					}
					const bool active = camera == activeCamera;
					drawViewportOutline(*draw, editorViewProjection, image,
						editor::ViewportWorldCorners(
							_animationPreview.CameraViewProjection(camera, gameSize)),
						active ? IM_COL32(255, 190, 90, 255) : IM_COL32(225, 155, 70, 210),
						active ? 2.f : 1.25f);
				});
				const bool tilePainting = current.IsValid() &&
					editor::IsInSceneTree(Selected(), current.Root()) &&
					drawTilemapBrush(editorViewProjection, image);
				world.Each<Raycast2D>([&](Entity ray, Raycast2D &) {
					if (current.IsValid() && editor::IsInSceneTree(ray, current.Root())) {
						if (const auto line = editor::RaycastGizmoLine(ray)) {
							drawViewportLine(*draw, editorViewProjection, image, *line,
								IM_COL32(100, 210, 255, 210), 1.5f);
						}
					}
				});
				bool gizmoInput = current.IsValid() &&
					editor::IsInSceneTree(Selected(), current.Root()) &&
					drawRaycastGizmo(editorViewProjection, image);
				gizmoInput = drawUIGizmo(image, gameSize, hoveredEntity) || gizmoInput;
				if (!Selected().Has<UIRect>()) {
					gizmoInput = drawNinePatchGizmo(editorViewProjection, image) || gizmoInput;
				}
				if (tilePainting) {
					finishTransformGizmo();
				}
				if (!_ninePatchBefore && (multiSelected() || !Selected().Has<UIRect>()) &&
					_gizmoMode != editor::GizmoMode::None && !tilePainting && !gizmoInput &&
					current.IsValid() && editor::IsInSceneTree(Selected(), current.Root())) {
					gizmoInput = drawTransformGizmo(editorViewProjection, image) || gizmoInput;
					if (!editor::CanEdit(Selected(), false)) {
						if (const auto position = editor::SelectionWorldPosition(Selected())) {
							drawSelectionMarker(*draw, editorViewProjection, image, *position);
						}
					}
				}
				draw->PopClipRect();

				if (!tilePainting && !gizmoInput && !_gizmoDrag) {
					drawBoxSelection(image, gameSize, hoveredEntity, current, resources);
				}
				for (Entity member : selection().Members()) {
					if (const auto *rect = member.Get<UIRect>()) {
						std::array<ImVec2, 4> points;
						for (std::size_t i = 0; i < 4; ++i) {
							points[i] =
								ImVec2(image.x + rect->viewportCorners[i].x * image.w / gameSize.x,
									image.y + rect->viewportCorners[i].y * image.h / gameSize.y);
						}
						draw->PushClipRect(min, max, true);
						draw->AddPolyline(points.data(), 4,
							member == Selected() ? IM_COL32(255, 220, 110, 255)
												 : IM_COL32(110, 190, 255, 255),
							ImDrawFlags_Closed, 2.f);
						draw->PopClipRect();
					}
					if (const auto position = editor::SelectionWorldPosition(member)) {
						const Vector2 point =
							editor::WorldToViewportImage(editorViewProjection, *position, image);
						draw->PushClipRect(min, max, true);
						draw->AddCircle(ImVec2(point.x, point.y), member == Selected() ? 9.f : 6.f,
							member == Selected() ? IM_COL32(255, 220, 110, 255)
												 : IM_COL32(110, 190, 255, 255),
							20, 2.f);
						draw->PopClipRect();
					}
				}

				if (!_marquee && !_gizmoDrag && !_raycastDrag && !_uiDragBefore &&
					!_ninePatchBefore && ImGui::IsItemHovered() &&
					ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					_cameraDragging = true;
				}
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
					_cameraDragging = false;
				}
				if (_cameraDragging) {
					const ImVec2 d = ImGui::GetIO().MouseDelta;
					_camera.position += editor::PanDelta(Vector2(d.x, d.y), _camera.zoom, gameSize,
						Vector2(_gameImageRect.w, _gameImageRect.h));
				}
				if (!_marquee && !_gizmoDrag && !_raycastDrag && !_uiDragBefore &&
					!_ninePatchBefore && ImGui::IsItemHovered()) {
					const f32 wheel = ImGui::GetIO().MouseWheel;
					if (wheel != 0.f) {
						const f32 oldZoom = _camera.zoom;
						_camera.zoom = editor::ZoomAfterScroll(oldZoom, wheel);
						const ImVec2 mouse = ImGui::GetIO().MousePos;
						_camera.position +=
							editor::ZoomCursorPan(Vector2(mouse.x - min.x, mouse.y - min.y),
								Vector2(_gameImageRect.w, _gameImageRect.h), gameSize, oldZoom,
								_camera.zoom);
					}
				}
			}
		}
	}

	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::End();
}

bool Editor::drawRaycastGizmo(const glm::mat4 &viewProjection, const Rect &image) {
	if (multiSelected()) {
		return false;
	}
	const auto line = editor::RaycastGizmoLine(Selected());
	if (!line) {
		return false;
	}
	const Vector2 endpoint = editor::WorldToViewportImage(viewProjection, line->b, image);
	ImDrawList *draw = ImGui::GetWindowDrawList();
	draw->AddCircleFilled(ImVec2(endpoint.x, endpoint.y), 5.f, IM_COL32(100, 210, 255, 255));
	const auto mouse = ImGui::GetIO().MousePos;
	const Vector2 point{mouse.x, mouse.y};
	const bool hovered = endpoint.DistanceTo(point) <= 9.f && point.x >= image.x &&
						 point.x <= image.x + image.w && point.y >= image.y &&
						 point.y <= image.y + image.h;
	if (!_raycastDrag && !_gizmoDrag && hovered && ImGui::IsWindowHovered() &&
		ImGui::IsMouseClicked(ImGuiMouseButton_Left) && editor::CanEdit(Selected(), false)) {
		_raycastDrag = RaycastDrag{_editingScene, Selected(),
			Selected().Get<Raycast2D>()->targetPosition, viewProjection, image};
	}
	if (_raycastDrag) {
		if (const auto target =
				editor::RaycastTargetFromViewport(Selected(), viewProjection, image, point)) {
			Selected().Get<Raycast2D>()->targetPosition = *target;
		}
		return true;
	}
	return hovered;
}

void Editor::finishRaycastGizmo(bool cancel) {
	if (!_raycastDrag) {
		return;
	}
	auto drag = *_raycastDrag;
	_raycastDrag.reset();
	if (auto *ray = drag.entity.Get<Raycast2D>()) {
		if (!cancel && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsMousePosValid()) {
			const auto mouse = ImGui::GetIO().MousePos;
			if (const auto target = editor::RaycastTargetFromViewport(drag.entity,
					drag.viewProjection, drag.image, {mouse.x, mouse.y})) {
				ray->targetPosition = *target;
			}
		}
		if (cancel) {
			ray->targetPosition = drag.before;
		} else if (_editingWorld != nullptr) {
			const auto type = _editingWorld->FindComponent("Raycast2D");
			if (const auto *field = type.FindField("targetPosition")) {
				editor::RecordFieldEdit(_sceneSaveStates, drag.scene, drag.entity, type, *field,
					editor::FieldValue(drag.before), editor::FieldValue(ray->targetPosition));
			}
		}
	}
}

bool Editor::drawBoxSelection(const Rect &image, Vector2 gameSize, Entity hovered, Scene scene,
	ResourceRegistry &resources) {
	const auto &io = ImGui::GetIO();
	const Vector2 mouse(io.MousePos.x, io.MousePos.y);
	if (_marquee &&
		(_marquee->root != scene.Root() ||
			(_marquee->image.x != image.x || _marquee->image.y != image.y ||
				_marquee->image.w != image.w || _marquee->image.h != image.h) ||
			ImGui::IsKeyPressed(ImGuiKey_Escape))) {
		// The marquee has not changed membership yet; preserve any newly active tab.
		_marquee.reset();
		return true;
	}
	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		commitPendingSceneEdits(resources);
		auto mode = editor::SelectionMode::Replace;
		if (io.KeyCtrl || io.KeySuper) {
			mode = editor::SelectionMode::Toggle;
		} else if (io.KeyShift) {
			mode = editor::SelectionMode::Add;
		}
		if (editor::IsInSceneTree(hovered, scene.Root())) {
			selection().Click(hovered, mode);
		} else if (scene.IsValid()) {
			_marquee = Marquee{mouse, image, selection(), scene.Root(), mode};
		}
	}
	if (!_marquee) {
		return false;
	}
	auto &drag = *_marquee;
	drag.dragging = drag.dragging || mouse.DistanceTo(drag.start) >= 4.f;
	if (drag.dragging) {
		const auto clamp = [&](Vector2 p) {
			return ImVec2(std::clamp(p.x, image.x, image.x + image.w),
				std::clamp(p.y, image.y, image.y + image.h));
		};
		const auto a = clamp(drag.start), b = clamp(mouse);
		auto *draw = ImGui::GetWindowDrawList();
		const ImVec2 lo(std::min(a.x, b.x), std::min(a.y, b.y)),
			hi(std::max(a.x, b.x), std::max(a.y, b.y));
		draw->AddRectFilled(lo, hi, IM_COL32(80, 150, 255, 35));
		draw->AddRect(lo, hi, IM_COL32(110, 190, 255, 255));
	}
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		if (drag.dragging) {
			const auto normalized = [&](Vector2 p) {
				return Vector2((p.x - image.x) / image.w, (p.y - image.y) / image.h);
			};
			_boxRequest = BoxSelectionRequest{scene.Root(), normalized(drag.start),
				normalized(mouse), gameSize, _pickTargetRevision, drag.mode, drag.before};
		} else if (drag.mode == editor::SelectionMode::Replace) {
			selection().Clear();
		}
		_marquee.reset();
	}
	return true;
}

void Editor::CompleteBoxSelection(const BoxSelectionRequest &request,
	const std::vector<Entity> &hits) {
	if (_selectionPlaying || !_selectionContext || _selectionContext->root != request.root ||
		!request.root.IsAlive() || selection() != request.before) {
		return;
	}
	std::vector<Entity> ordered;
	const auto visit = [&](const auto &self, Entity entity) -> void {
		if (std::find(hits.begin(), hits.end(), entity) != hits.end()) {
			ordered.push_back(entity);
		}
		for (Entity child : entity.GetChildren()) {
			self(self, child);
		}
	};
	visit(visit, request.root);
	selection() = request.before;
	if (request.mode == editor::SelectionMode::Replace) {
		selection().Clear();
	}
	for (Entity entity : ordered) {
		selection().Click(entity,
			request.mode == editor::SelectionMode::Toggle ? editor::SelectionMode::Toggle
														  : editor::SelectionMode::Add);
	}
	selection().Prune(request.root);
}
} // namespace sowa
