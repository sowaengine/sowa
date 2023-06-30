#include "model_builder.hxx"

bool ModelBuilder::Quad2D(Model &model, float size) {
	model.New();

	float sz = size / 2.f;

	const std::vector<float> vertices = {
		sz, sz, 0.0f, /**/ 1.0, 1.0,   // top right
		sz, -sz, 0.0f, /**/ 1.0, 0.0,  // bottom right
		-sz, -sz, 0.0f, /**/ 0.0, 0.0, // bottom left
		-sz, sz, 0.0f, /**/ 0.0, 1.0   // top left
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