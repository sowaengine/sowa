#ifndef SW_MODEL_BUILDER_HXX
#define SW_MODEL_BUILDER_HXX
#pragma once

#include "model.hxx"

class ModelBuilder {
  public:
	static bool Quad2D(Model &model, float size = 1.0f);
	static bool Quad2D(Model &model, float x, float y, float w, float h);
	static bool UIQuad(Model &model, float x, float y, float w, float h, float r, float g, float b, float a, float id);

  private:
	ModelBuilder() = default;
};

#endif // SW_MODEL_BUILDER_HXX