#ifndef _E_TEXT2D_HPP__
#define _E_TEXT2D_HPP__

#include "scene/node.hpp"
#include "math/math.hpp"
#include "scene/2d/node2d.hpp"

#include "resource/font/font.hpp"
#include "gfx/font.hpp"

namespace sowa {
class Text2D : public Node2D {
  public:
	Text2D();

	static std::string Typename() { return "sowa::Text2D"; }
	static void Bind();

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() { return m_text; }

	static FileBuffer SaveImpl(object_type* out);
	static bool LoadImpl(object_type* out, const FileBuffer& buf);
  private:

    std::string m_text{""};
	Reference<gfx::IFont> m_font;
};
} // namespace sowa

#endif // _E_TEXT2D_HPP__