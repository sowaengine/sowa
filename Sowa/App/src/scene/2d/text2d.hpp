#ifndef _E_TEXT2D_HPP__
#define _E_TEXT2D_HPP__

#include "scene/node.hpp"
#include "utils/math.hpp"
#include "scene/2d/node2d.hpp"

#include "resource/font/font.hpp"

namespace Sowa {
class Text2D : public Node2D {
  public:
	Text2D();

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

    void SetText(const std::string& text) { _Text = text; }
    const std::string& GetText() { return _Text; }
  private:

    std::string _Text{""};
	Reference<Font> _Font;
};
} // namespace Sowa

#endif // _E_TEXT2D_HPP__