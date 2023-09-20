#ifndef SW_SPRITE_2D_HXX
#define SW_SPRITE_2D_HXX
#pragma once

#include "node_2d.hxx"

#include "resource/resource_type.hxx"
#include <memory>

class Sprite2D : public Node2D {
  public:
	void _update() override;
	virtual ~Sprite2D() = default;

	inline RID &texture() { return m_texture; }

  protected:
	RID m_texture;
};

#endif // SW_SPRITE_2D_HXX