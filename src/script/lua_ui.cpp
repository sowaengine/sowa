#include "script/lua_ui.hpp"
#include "core/log.hpp"
#include "core/scope_exit.hpp"
#include "ecs/world.hpp"
#include "script/script_server.hpp"
#include "ui/ui_system.hpp"

namespace sowa {
void BindUI(ScriptServer &scripts, UISystem &ui) {
	auto &lua = scripts.Lua();
	sol::table table = lua.create_named_table("ui");
	table.set_function("create",
		[&scripts](Entity parent, const std::string &kind, sol::optional<std::string> name) {
			auto &world = scripts.GetWorld();
			const bool deferred = world.IsDeferred();
			if (deferred) {
				world.SuspendDefer();
			}
			auto resume = MakeScopeExit([&] {
				if (deferred) {
					world.ResumeDefer();
				}
			});
			return CreateUI(parent, kind, name.value_or(""));
		});
	table.set_function("anchor", [](Entity e, const std::string &preset) {
		if (e.IsAlive()) {
			if (auto *rect = e.Get<UIRect>()) {
				SetUIAnchorPreset(*rect, preset);
			}
		}
	});
	table.set_function("focus", [&ui](Entity e) { return ui.Focus(e); });
	table.set_function("focused", [&ui]() { return ui.Focused(); });
	table.set_function("captures_pointer", [&ui]() { return ui.CapturesPointer(); });
	table.set_function("captures_keyboard", [&ui]() { return ui.CapturesKeyboard(); });
	table.set_function("open", [&ui](Entity e) { ui.Open(e, true); });
	table.set_function("close", [&ui](Entity e) { ui.Open(e, false); });
	table.set_function("scroll_to", [&ui](Entity e, Vector2 offset) { ui.ScrollTo(e, offset); });
	table.set_function("ensure_visible", [&ui](Entity e) { ui.EnsureVisible(e); });
	table.set_function("set_items", [&ui](Entity e, sol::table values) {
		std::vector<std::string> items;
		for (size_t i = 1; i <= values.size(); ++i) {
			items.push_back(values.get<std::string>(i));
		}
		ui.SetItems(e, items);
	});
	table.set_function("off", [&ui](u64 id) { ui.Off(id); });
	table.set_function("on",
		[&ui, &lua](Entity e, const std::string &type, sol::protected_function callback) {
			return ui.On(e, type, [callback, &lua](UIEvent &event) {
				sol::table data = lua.create_table();
				data["type"] = event.type;
				data["target"] = event.target;
				data["current_target"] = event.currentTarget;
				data["position"] = event.position;
				data["delta"] = event.delta;
				data["value"] = event.value;
				data["text"] = event.text;
				data["handled"] = false;
				sol::protected_function_result result = callback(data);
				if (!result.valid()) {
					sol::error error = result;
					Log::Error("UI event '{}': {}", event.type, error.what());
					return;
				}
				event.handled = data.get_or("handled", false) ||
								(result.get_type() == sol::type::boolean && result.get<bool>());
			});
		});
}
} // namespace sowa
