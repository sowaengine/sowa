#pragma once

#include "imgui.h"

namespace sowa::editor {

// Shared semantic colors for editor chrome and type-specific accents.
namespace theme {
inline const ImVec4 Accent{0.25f, 0.77f, 0.85f, 1.f};
inline const ImVec4 Blue{0.47f, 0.64f, 0.69f, 1.f};
inline const ImVec4 Green{0.51f, 0.79f, 0.66f, 1.f};
inline const ImVec4 Amber{0.91f, 0.74f, 0.47f, 1.f};
inline const ImVec4 Rose{0.91f, 0.55f, 0.61f, 1.f};
// Shared by the native title bar and the in-editor menu strip.
inline const ImVec4 TitleStart{0.f, 0.f, 0.f, 1.f};
inline const ImVec4 TitleMiddle{0.f, 0.080f, 0.100f, 1.f};
inline const ImVec4 TitleEnd{0.f, 0.320f, 0.400f, 1.f};
inline const ImVec4 TitleDivider{0.f, 0.72f, 0.82f, 0.35f};
} // namespace theme

// Applies the editor's charcoal surfaces, cyan selection and compact spacing.
void ApplyStyle(ImGuiStyle &style);

// Keep menu targets comfortable even when opened from a compact tree panel.
struct MenuStyle {
	MenuStyle();
	~MenuStyle();
	MenuStyle(const MenuStyle &) = delete;
	MenuStyle &operator=(const MenuStyle &) = delete;
};

// Dialog spacing is independent of the compact panel that opens it.
struct DialogStyle {
	DialogStyle();
	~DialogStyle();
	DialogStyle(const DialogStyle &) = delete;
	DialogStyle &operator=(const DialogStyle &) = delete;
};

// Native tree behavior with independently tinted icons and a selection marker.
// Leaves the tree row as ImGui's last item for click/context/drag handling.
bool TreeRow(const char *id, const char *label, const char *icon, const ImVec4 &tint,
	ImGuiTreeNodeFlags flags, float trailingWidth = 0.f);
// Place a button in the reserved row area without changing the cursor/tree
// indentation. Call after processing the tree row's click/drag/context actions.
bool TreeRowButton(const char *id, const char *icon, const char *tooltip, ImVec2 rowMin,
	ImVec2 rowMax, bool enabled = true);
bool SectionHeader(const char *label, const char *icon, const ImVec4 &tint = theme::Accent);

} // namespace sowa::editor
