#ifndef SW_MODEL_BUILDER_HXX
#define SW_MODEL_BUILDER_HXX
#pragma once

#include "model.hxx"

class ModelBuilder {
  public:
	static bool Quad2D(Model &model);

  private:
	ModelBuilder() = default;
};

#endif // SW_MODEL_BUILDER_HXX