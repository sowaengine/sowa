#ifndef _E_INPUT_EVENT_HPP__
#define _E_INPUT_EVENT_HPP__

namespace sowa {
namespace gfx {
class Window;
}
} // namespace sowa

namespace sowa {
class WindowAccessor;

enum class InputEventType {
	None = 0,
	Scroll,
	MouseMove,
	MouseButton,
	MouseClick,
	Key,
	Character,
};

struct InputEvent {
  public:
	inline InputEventType Type() { return _Type; }

	struct {
		double scrollX;
		double scrollY;
	} scroll;

	struct {
		double deltaX;
		double deltaY;
		double posX;
		double posY;
	} mouseMove;

	struct {
		int button;
		int action;
		int modifiers;
	} mouseButton;

	struct {
		int button;
		bool single;
		int modifiers;
	} mouseClick;

	struct {
		int key;
		int scanCode;
		int action;
		int modifiers;
	} key;

	struct {
		unsigned int codePoint;
	} character;

  private:
	friend class WindowAccessor;
	friend class sowa::gfx::Window;

	InputEventType _Type;
};

} // namespace sowa

#endif // _E_INPUT_EVENT_HPP__