#ifndef SW_BUTTON_HPP_
#define SW_BUTTON_HPP_

#include <string>

#include "scene/2d/nine_slice_panel.hpp"

namespace sowa {
class Button : public NineSlicePanel {
  public:
	Button();

	static std::string Typename() { return "sowa::Button"; }

	void EnterScene() override;
	void ExitScene() override;
	void UpdateLogic() override;
	void UpdateDraw() override;

    inline std::string& Text() { return m_text; }

	static FileBuffer SaveImpl(object_type *out);
	static bool LoadImpl(object_type *out, const FileBuffer &buf);

  private:
	friend class Application;
	
    std::string m_text{""};
};
} // namespace sowa

#endif // SW_BUTTON_HPP_