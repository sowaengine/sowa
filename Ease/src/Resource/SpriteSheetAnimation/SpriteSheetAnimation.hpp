#ifndef _E_SPRITESHEETANIMATION_HPP__
#define _E_SPRITESHEETANIMATION_HPP__

#pragma once

#include <functional>
#include <stdint.h>

#include "Ease.hpp"
#include "Resource/Resource.hpp"

namespace Ease {
class SpriteSheetAnimation : public BaseResource {
  public:
	SpriteSheetAnimation();
	~SpriteSheetAnimation();

	inline int &HFrames() { return m_hFrames; }
	inline int &VFrames() { return m_vFrames; }
	inline int &SelectedRow() { return m_SelectedRow; }
	inline int &FrameCount() { return m_FrameCount; }
	inline int &StartFrame() { return m_StartFrame; }
	inline int &FPS() { return m_Fps; }

	inline ResourceID &Texture() { return m_Texture; }

  private:
	template <typename>
	friend class ResourceLoaderImpl;
	int m_hFrames;
	int m_vFrames;
	int m_SelectedRow;
	int m_FrameCount;
	int m_StartFrame;
	int m_Fps;

	ResourceID m_Texture;
};
} // namespace Ease

#endif