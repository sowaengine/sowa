#include "model_builder.hxx"

#define POS(x, y, z) x, y, z
#define UV(u, v) u, v
#define COLOR(r, g, b, a) r, g, b, a
#define ID(id) id

bool ModelBuilder::Quad2D(Model &model, float size) {
	model.New();

	float sz = size / 2.f;

	const std::vector<float> vertices = {
		POS(sz, sz, 0.0f), /****/ UV(1.0, 1.0), // top right
		POS(sz, -sz, 0.0f), /***/ UV(1.0, 0.0), // bottom right
		POS(-sz, -sz, 0.0f), /**/ UV(0.0, 0.0), // bottom left
		POS(-sz, sz, 0.0f), /***/ UV(0.0, 1.0)	// top left
	};
	const std::vector<uint32_t> indices = {
		0, 1, 3,
		1, 2, 3};

	model.SetModelData(vertices);
	model.SetIndexData(indices);

	model.ResetAttributes();
	model.SetAttribute(0, gfx::AttributeType::Vec3);
	model.SetAttribute(1, gfx::AttributeType::Vec2);
	model.UploadAttributes();

	return true;
}

bool ModelBuilder::Quad2D(Model &model, float x, float y, float w, float h) {
	model.New();

	const std::vector<float> vertices = {
		POS(x + w, y + h, 0.0f), /**/ UV(1.0, 1.0), // top right
		POS(x + w, y, 0.0f), /******/ UV(1.0, 0.0), // bottom right
		POS(x, y, 0.0f), /**********/ UV(0.0, 0.0), // bottom left
		POS(x, y + h, 0.0f), /******/ UV(0.0, 1.0)	// top left
	};

	const std::vector<uint32_t> indices = {
		0, 1, 3,
		1, 2, 3};

	model.SetModelData(vertices);
	model.SetIndexData(indices);

	model.ResetAttributes();
	model.SetAttribute(0, gfx::AttributeType::Vec3);
	model.SetAttribute(1, gfx::AttributeType::Vec2);
	model.UploadAttributes();

	return true;
}

bool ModelBuilder::UIQuad(Model &model, float x, float y, float w, float h, float r, float g, float b, float a, float id) {
	model.New();

	const std::vector<float> vertices = {
		POS(x + w, y + h, 0.0f), /**/ UV(1.0, 1.0), COLOR(r, g, b, a), ID(id), // top right
		POS(x + w, y, 0.0f), /******/ UV(1.0, 0.0), COLOR(r, g, b, a), ID(id), // bottom right
		POS(x, y, 0.0f), /**********/ UV(0.0, 0.0), COLOR(r, g, b, a), ID(id), // bottom left
		POS(x, y + h, 0.0f), /******/ UV(0.0, 1.0), COLOR(r, g, b, a), ID(id)  // top left
	};

	const std::vector<uint32_t> indices = {
		0, 1, 3,
		1, 2, 3};

	model.SetModelData(vertices);
	model.SetIndexData(indices);

	model.ResetAttributes();
	model.SetAttribute(0, gfx::AttributeType::Vec3);
	model.SetAttribute(1, gfx::AttributeType::Vec2);
	model.SetAttribute(2, gfx::AttributeType::Vec4);
	model.SetAttribute(3, gfx::AttributeType::Float);
	model.UploadAttributes();

	return true;
}