#ifndef _E_INPUT_EVENT_HPP__
#define _E_INPUT_EVENT_HPP__

namespace Sowa {
class WindowAccessor;

enum class InputEventType {
	None = 0,
	Scroll,
	MouseMove,
	MouseButton,
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
	InputEventType _Type;
};

} // namespace Sowa

#endif // _E_INPUT_EVENT_HPP__