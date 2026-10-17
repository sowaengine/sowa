#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/input.hpp"
#include "core/project_settings.hpp"
#include "core/types.hpp"
#include "ecs/entity.hpp"
#include "editor/animation_preview.hpp"
#include "editor/animation_timeline.hpp"
#include "editor/camera.hpp"
#include "editor/command_palette.hpp"
#include "editor/console.hpp"
#include "editor/filesystem_browser.hpp"
#include "editor/gizmos.hpp"
#include "editor/hierarchy.hpp"
#include "editor/profiler_panel.hpp"
#include "editor/project_export_panel.hpp"
#include "editor/project_settings_panel.hpp"
#include "editor/resource_inspector.hpp"
#include "editor/scene_commands.hpp"
#include "editor/scene_documents.hpp"
#include "editor/sprite_sheet_panel.hpp"
#include "math/rect.hpp"
#include "math/vector2.hpp"

namespace sowa {

class Window;
class FileSystem;
class GraphicsDevice;
class Renderer2D;
class World;
class SceneManager;
class ScriptServer;
class ScriptSuspension;
class ResourceRegistry;

// In-engine editor UI, document tools, viewport, and play controls.
// Initialized only when Application enables the editor before Init.
class Editor {
  public:
	Editor() = default;
	~Editor();

	Editor(const Editor &) = delete;
	Editor &operator=(const Editor &) = delete;
	Editor(Editor &&) = delete;
	Editor &operator=(Editor &&) = delete;

	bool Init(Window &window, FileSystem &fs);
	void Shutdown();

	bool IsInitialized() const {
		return _initialized;
	}

	void BeginFrame();
	// gameTextureId is GraphicsDevice::GetNativeTextureId of the game
	// color attachment. gameSize is the fixed viewport resolution.
	// Pan/zoom run only while playing is false (edit session, not play).
	void Draw(u64 gameTextureId, Vector2 gameSize, bool playing, World &world, SceneManager &scenes,
		ProjectSettings &project, ScriptServer &scripts, ResourceRegistry &resources,
		Entity hoveredEntity);
	void EndFrame(GraphicsDevice &device, Window &window);
	// Ends the ImGui frame without rendering. A mid-frame breakpoint suspends it while
	// the editor runs complete frames; BeginFrame opens the outer frame again on resume.
	void SuspendFrame();

	// Displays the paused source location with Continue and Stop controls.
	void SetScriptSuspension(ScriptSuspension *suspension) {
		_scriptSuspension = suspension;
	}

	// Displays the debugger connection banner and Don't wait button while Play waits
	// for VS Code. An empty value hides it.
	void SetDebuggerWait(std::optional<u16> port) {
		_debuggerWaitPort = port;
		_debuggerWaitCancelled = false;
	}

	// True once after "Don't wait" was pressed.
	bool TakeDebuggerWaitCancel() {
		return std::exchange(_debuggerWaitCancelled, false);
	}

	void ApplyAnimationPreview(World &world, ResourceRegistry &resources, f64 delta, bool playing,
		Vector2 viewportSize);

	void RestoreAnimationPreview(World &world) {
		_animationPreview.Restore(world);
	}

	void StopAnimationPreview() {
		_animationPreview.Stop();
	}

	// Project Settings requests are consumed by Application at the start of
	// the next frame. Deferring avoids destroying a game texture that ImGui's
	// current draw data still references.
	std::optional<ProjectSettings> TakeProjectSettingsSaveRequest();
	void ReportProjectSettingsSaveResult(bool success, std::string message);

	bool CanReloadScene(Scene scene) const {
		return !editor::IsSceneDocumentUnsaved(_sceneSaveStates, scene);
	}

	Entity Selected() const {
		return _selected;
	}

	void SetSelected(Entity entity) {
		_selected = entity;
	}

	// Debug menu toggle. Application applies it to PhysicsWorld each frame.
	bool ShowPhysicsDebug() const {
		return _showPhysicsDebug;
	}

	bool WantsKeyboard() const;
	bool WantsMouse() const;
	// True only while an ImGui text field is active. Engine hotkeys (F5–F9)
	// must still run when a docked window is focused - WantCaptureKeyboard
	// is true in that case because NavEnableKeyboard is on.
	bool WantsTextInput() const;
	bool CapturesGameKeyboard() const;

	// Last frame's game image in window-content pixels (origin top-left).
	// Empty until the first Draw. Application maps this for Input.
	const Rect &GameImageRect() const {
		return _gameImageRect;
	}

	const editor::EditorCamera &Camera() const {
		return _camera;
	}

	bool IsCameraPreviewEnabled() const {
		return _previewSceneCamera;
	}

	void SetCameraView(Vector2 position, f32 zoom);
	void DrawViewportGrid(Renderer2D &renderer, Vector2 viewportSize);

  private:
	void drawCommandPalette(SceneManager &scenes, ProjectSettings &project, ScriptServer &scripts,
		ResourceRegistry &resources);
	editor::CommandPalette _commandPalette;
	void drawDockHost(SceneManager &scenes, const ProjectSettings &project,
		ResourceRegistry &resources);
	void drawPlayControls(SceneManager &scenes, const ProjectSettings &project,
		ResourceRegistry &resources);
	void drawViewport(u64 gameTextureId, Vector2 gameSize, bool playing, World &world,
		SceneManager &scenes, ResourceRegistry &resources, Entity hoveredEntity);
	void drawSceneTabs(SceneManager &scenes, ResourceRegistry &resources, bool playing);
	void drawHierarchy(World &world, SceneManager &scenes, ResourceRegistry &resources,
		bool playing);
	void drawHierarchyNode(Entity entity, ResourceRegistry &resources, bool playing,
		bool defaultOpen);
	std::string _openInstanceSceneRequested;
	void drawCreateMenu(Entity parent, bool enabled, bool submenu = true);
	void drawSceneInstancePicker(World &world, ResourceRegistry &resources, bool playing);
	void drawTilemapInspector(ResourceRegistry &resources, bool playing);
	void drawTilemapEditor(ResourceRegistry &resources, bool playing);
	bool drawTilemapLayers(Tilemap &map, const std::function<void()> &capture);
	void drawTilemapPalette(const Tilemap &map, ResourceRegistry &resources);
	bool drawTilemapSources(ResourceRegistry &resources, const std::function<void()> &capture);
	void drawParticlesInspector(World &world, ResourceRegistry &resources, bool playing,
		bool editable);
	void drawAnimationInspector(ResourceRegistry &resources, bool playing);
	void drawAnimationEditor(World &world, ResourceRegistry &resources, bool playing);
	void drawSpriteSheetEditor(ResourceRegistry &resources, bool playing);
	void drawNinePatchEditor(World &world, ResourceRegistry &resources, bool playing);
	bool drawNinePatchGizmo(const glm::mat4 &viewProjection, const Rect &image);
	void finishNinePatchResize(bool cancel = false);
	void finishNinePatchPadding(bool cancel = false);
	void drawAddComponent(World &world, ScriptServer &scripts, ResourceRegistry &resources,
		bool playing);
	void createAnimationLibrary(ResourceRegistry &resources);
	bool drawTilemapBrush(const glm::mat4 &viewProjection, const Rect &image);
	void finishTileStroke();
	bool drawTransformGizmo(const glm::mat4 &viewProjection, const Rect &image);
	void drawGizmoToolbar(bool disabled);
	void finishTransformGizmo(bool cancel = false);
	bool drawRaycastGizmo(const glm::mat4 &viewProjection, const Rect &image);
	void finishRaycastGizmo(bool cancel = false);
	bool drawComponentHeader(const char *typeName, const char *label, const char *icon,
		bool playing);
	std::string _removeComponentRequested;
	void drawEntityActions(Entity entity, Entity pasteParent, bool playing,
		ResourceRegistry &resources);
	void copyEntity(ResourceRegistry &resources);
	void pasteEntity(Entity parent, ResourceRegistry &resources);
	void duplicateEntity(ResourceRegistry &resources);
	void drawInspector(World &world, SceneManager &scenes, ScriptServer &scripts,
		ProjectSettings &project, ResourceRegistry &resources, bool playing);
	void drawInspectorField(Entity entity, ComponentType type, const FieldInfo &field,
		bool editable, World &world, ResourceRegistry &resources, float labelWidth = 140.f);
	void finishInspectorFieldEdit(Entity entity, ComponentType type, const FieldInfo &field,
		const editor::FieldValue &before);
	void commitPendingSceneEdits(ResourceRegistry &resources);
	void drawFilesystem(SceneManager &scenes, bool playing);
	void drawFilesystemDir(FileSystem &fs, SceneManager &scenes, const std::string &dirPath,
		bool playing, bool defaultOpen);
	void drawFilesystemContext(SceneManager &scenes, const std::string &path,
		editor::AssetKind kind, bool playing);
	void drawFilesystemCreateMenu(const std::string &directory, bool playing);
	void drawFilesystemDialog(SceneManager &scenes, bool playing);
	void drawConsole();

	enum class FilesystemDialogAction {
		None,
		CreateScene,
		CreateScript,
		CreateFile,
		CreateDirectory,
		Rename,
		Delete,
	};
	void requestFilesystemDialog(FilesystemDialogAction action, const std::string &path,
		editor::AssetKind kind = editor::AssetKind::Other);

	World *_editingWorld = nullptr;
	Window *_window = nullptr;
	FileSystem *_fileSystem = nullptr;
	bool _initialized = false;
	bool _showStyleEditor = false;
	bool _showPhysicsDebug = false;
	bool _showProfiler = false;
	ScriptSuspension *_scriptSuspension = nullptr;
	std::optional<u16> _debuggerWaitPort;
	bool _debuggerWaitCancelled = false;
	void drawDebuggerWait();
	void drawScriptSuspension(SceneManager &scenes, const ProjectSettings &project);
	editor::ProfilerPanel _profilerPanel;
	editor::ProjectSettingsPanel _projectSettings;
	editor::ProjectExportPanel _projectExport;
	bool _applyDefaultDock = true;
	std::string _selectedAsset;
	FilesystemDialogAction _filesystemDialogAction = FilesystemDialogAction::None;
	std::string _filesystemDialogPath;
	editor::AssetKind _filesystemDialogAssetKind = editor::AssetKind::Other;
	char _filesystemDialogName[256]{};
	std::string _filesystemDialogError;
	std::string _scriptInspectorError;
	Entity _sceneInstanceParent;
	Entity _expandHierarchyEntity;
	std::string _sceneInstanceError;
	bool _openSceneInstancePicker = false;
	bool _openFilesystemDialog = false;
	editor::LogFilter _consoleFilter;
	bool _consoleAutoScroll = true;
	Rect _gameImageRect;
	editor::EditorCamera _camera;
	bool _cameraDragging = false;
	bool _previewSceneCamera = false;
	bool drawUIGizmo(const Rect &image, Vector2 gameSize, Entity hoveredEntity);
	void finishUIGizmo(bool cancel = false);
	bool _showNinePatchEditor = false;
	std::string _ninePatchType = "NinePatchRect";
	std::optional<NinePatchRect> _ninePatchBefore;
	Entity _ninePatchEntity;
	Scene _ninePatchScene;
	glm::mat4 _ninePatchMapping{1.f};
	Vector2 _ninePatchStart;
	Vector2 _ninePatchCorner;
	Entity _uiDragEntity;
	Scene _uiDragScene;
	std::optional<UIRect> _uiDragBefore;
	Vector2 _uiDragStart;
	Vector2 _uiResizeCorner{-1, -1};
	glm::mat4 _uiDragMapping{1.f};
	f32 _uiDragScale = 1;
	editor::GizmoMode _gizmoMode = editor::GizmoMode::Position;
	std::optional<editor::GizmoDrag> _gizmoDrag;

	struct RaycastDrag {
		Scene scene;
		Entity entity;
		Vector2 before;
		glm::mat4 viewProjection;
		Rect image;
	};

	std::optional<RaycastDrag> _raycastDrag;
	Entity _gizmoEntity;
	Scene _gizmoScene;
	Entity _selected;
	editor::PlaySelection _playSelection;
	editor::AnimationPreview _animationPreview;
	editor::AnimationTimeline _animationTimeline;
	bool _showAnimationEditor = false;
	bool _showSpriteSheetEditor = false;
	editor::SpriteSheetPanel _spriteSheetPanel;
	char _componentSearch[128]{};
	bool _tilePainting = false;
	bool _showTilemapEditor = false;
	bool _tileErasing = false;
	i32 _tileLayer = 0;
	i32 _tileBrush = 0;

	struct TileSourceForm {
		i32 kind = 0; // sheet, image, scene
		std::string path;
		Vector2 tileSize{16, 16};
		i32 margin = 0;
		i32 spacing = 0;
		Rect region;
	};

	void drawTileSourceForm(TileSourceForm &source, bool adding);
	bool applyTileSource(ResourceRegistry &resources, TileSourceForm &source, bool adding,
		const std::function<void()> &capture);
	TileSourceForm _tileAddSource;
	TileSourceForm _tileEditSource;
	Entity _tileSourceEntity;
	i32 _tileSourceBrush = 0;
	bool _tileEditWholeSheet = false;
	bool _tileSourceNeedsRefresh = true;
	std::string _tileImportError;
	Entity _tileStrokeEntity;
	Scene _tileStrokeScene;
	std::optional<Tilemap> _tileStrokeBefore;
	std::optional<TileCell> _tileStrokeLast;
	bool _tileStrokeChanged = false;
	Entity _visibleSceneTab;
	Scene _sceneCloseRequest;
	bool _openSceneCloseDialog = false;
	std::vector<Entity> _sceneTabOrder;
	std::vector<editor::SceneDocumentSaveState> _sceneSaveStates;
	bool _saveSceneRequested = false;
	bool _undoSceneRequested = false;
	bool _redoSceneRequested = false;
	Scene _editingScene;

	struct PendingFieldEdit {
		Scene scene;
		Entity entity;
		ComponentType type;
		std::string field;
		editor::FieldValue before;
	};

	editor::ResourceInspector _resourceInspector;
	std::optional<PendingFieldEdit> _pendingFieldEdit;
	std::optional<PendingFieldEdit> _paddingDrag;
	Entity _pendingRenameEntity;
	Scene _pendingRenameScene;
	std::string _pendingRenameBefore;
};

} // namespace sowa
