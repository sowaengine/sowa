#include "editor/editor.hpp"
#include "editor/inspector.hpp"
#include "editor/nine_patch_tools.hpp"
#include "render/image_patches.hpp"
#include "resource/image_texture.hpp"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace sowa {
namespace {
constexpr const char *paddingFields[] = {
	"paddingLeft",
	"paddingRight",
	"paddingTop",
	"paddingBottom",
};

void drawTexture(ImDrawList &draw, ImTextureRef texture, Rect area, Rect uv) {
	draw.AddImage(texture, {area.x, area.y}, {area.Right(), area.Top()}, {uv.x, uv.Top()},
		{uv.Right(), uv.y});
}
} // namespace

void Editor::finishNinePatchPadding(bool cancel) {
	if (!_paddingDrag) {
		return;
	}
	auto &drag = *_paddingDrag;
	if (drag.entity.IsAlive()) {
		const auto *field = drag.type.FindField(drag.field);
		auto *value = drag.entity.GetField(drag.type, drag.field).As<f32>();
		if (field && value) {
			if (cancel) {
				*value = std::get<f32>(drag.before);
			} else {
				editor::RecordFieldEdit(_sceneSaveStates, drag.scene, drag.entity, drag.type,
					*field, drag.before, *value);
			}
		}
	}
	_paddingDrag.reset();
}

void Editor::drawNinePatchEditor(World &world, ResourceRegistry &resources, bool playing) {
	if (!_showNinePatchEditor) {
		return;
	}
	ImGui::SetNextWindowSize({660, 640}, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Nine-patch padding", &_showNinePatchEditor)) {
		ImGui::End();
		return;
	}
	ComponentType type;
	for (const auto candidate : _selected.GetComponents()) {
		if (candidate.Name() == _ninePatchType) {
			type = candidate;
			break;
		}
	}
	if (!type.IsValid()) {
		ImGui::TextUnformatted("Select a nine-patch rectangle to edit its padding.");
		ImGui::End();
		return;
	}
	const bool editable = editor::CanEdit(_selected, playing);
	ImGui::BeginDisabled(!editable);
	for (const auto *name : paddingFields) {
		const auto *field = type.FindField(name);
		drawInspectorField(_selected, type, *field, editable, world, resources);
	}
	ImGui::EndDisabled();
	const auto id = *_selected.GetField(type, "texture").As<ResourceID>();
	const auto texture = resources.Get<ImageTexture>(id);
	if (!texture || !texture->IsLoaded()) {
		ImGui::TextWrapped(
			"Choose a texture in the component Inspector to edit its padding visually.");
		ImGui::End();
		return;
	}
	const Vector2 size = texture->Size();
	const ImTextureRef native(static_cast<ImTextureID>(
		resources.Context().device->GetNativeTextureId(texture->Handle())));
	ImGui::TextUnformatted("Drag a guide to set padding in pixels. Escape cancels the drag.");
	const auto available = ImGui::GetContentRegionAvail();
	const float zoom = std::max(.01f, std::min((available.x - 24) / size.x, 280.f / size.y));
	const auto origin = ImGui::GetCursorScreenPos();
	const Rect image{origin.x + 12, origin.y + 12, size.x * zoom, size.y * zoom};
	ImGui::InvisibleButton("Padding guides", {image.w + 24, image.h + 24});
	const bool hovered = ImGui::IsItemHovered();
	auto &draw = *ImGui::GetWindowDrawList();
	draw.AddRectFilled({image.x, image.y}, {image.Right(), image.Top()}, IM_COL32(32, 36, 44, 255));
	drawTexture(draw, native, image,
		texture->flipVertically ? Rect(0, 0, 1, 1) : Rect(0, 1, 1, -1));
	const auto mouse = ImGui::GetMousePos();
	const Vector2 point{(mouse.x - image.x) / zoom, (mouse.y - image.y) / zoom};
	int hoveredGuide = -1;
	float distance = 9;
	for (int i = 0; i < 4; ++i) {
		const float padding = *_selected.GetField(type, paddingFields[i]).As<f32>();
		const bool horizontal = i < 2;
		const float extent = horizontal ? size.x : size.y;
		const float position = std::clamp(i % 2 ? extent - padding : padding, 0.f, extent);
		const float coordinate = (horizontal ? image.x : image.y) + position * zoom;
		const ImVec2 a =
			horizontal ? ImVec2(coordinate, image.y - 6) : ImVec2(image.x - 6, coordinate);
		const ImVec2 b = horizontal ? ImVec2(coordinate, image.Top() + 6)
									: ImVec2(image.Right() + 6, coordinate);
		draw.AddLine(a, b, IM_COL32(90, 210, 250, 255), 2);
		const float delta = std::abs((horizontal ? mouse.x : mouse.y) - coordinate);
		if (hovered && delta < distance) {
			distance = delta;
			hoveredGuide = i;
		}
	}
	if (editable && hoveredGuide >= 0) {
		ImGui::SetMouseCursor(
			hoveredGuide < 2 ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
		ImGui::SetTooltip("%s", paddingFields[hoveredGuide]);
		if (!_paddingDrag && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			const auto *name = paddingFields[hoveredGuide];
			_paddingDrag = PendingFieldEdit{_editingScene, _selected, type, name,
				*_selected.GetField(type, name).As<f32>()};
		}
	}
	if (_paddingDrag && _paddingDrag->entity == _selected && _paddingDrag->type == type) {
		for (int i = 0; i < 4; ++i) {
			if (_paddingDrag->field == paddingFields[i]) {
				const float opposite = *_selected.GetField(type, paddingFields[i ^ 1]).As<f32>();
				*_selected.GetField(type, paddingFields[i]).As<f32>() =
					editor::NinePatchPaddingAt(paddingFields[i], point, size, opposite);
			}
		}
	}
	ImGui::TextUnformatted("Resized preview");
	const auto previewOrigin = ImGui::GetCursorScreenPos();
	const Rect preview{previewOrigin.x, previewOrigin.y, available.x, 120};
	ImagePatchOptions options;
	options.ninePatch = true;
	options.paddingLeft = *_selected.GetField(type, "paddingLeft").As<f32>();
	options.paddingRight = *_selected.GetField(type, "paddingRight").As<f32>();
	options.paddingTop = *_selected.GetField(type, "paddingTop").As<f32>();
	options.paddingBottom = *_selected.GetField(type, "paddingBottom").As<f32>();
	options.tileCenter = *_selected.GetField(type, "tileCenter").As<bool>();
	options.tileEdges = *_selected.GetField(type, "tileEdges").As<bool>();
	options.drawCenter = *_selected.GetField(type, "drawCenter").As<bool>();
	for (const auto &patch : BuildImagePatches(preview, size, texture->flipVertically, options)) {
		drawTexture(draw, native, patch.rect, patch.uv);
	}
	ImGui::Dummy({preview.w, preview.h});
	ImGui::End();
}

} // namespace sowa
