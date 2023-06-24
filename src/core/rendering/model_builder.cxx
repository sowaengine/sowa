#include "model_builder.hxx"

bool ModelBuilder::Quad2D(Model &model) {
	model.New();

	static const std::vector<float> vertices = {
		0.5f, 0.5f, 0.0f,	// top right
		0.5f, -0.5f, 0.0f,	// bottom right
		-0.5f, -0.5f, 0.0f, // bottom left
		-0.5f, 0.5f, 0.0f	// top left
	};
	static const std::vector<uint32_t> indices = {
		0, 1, 3,
		1, 2, 3};

	model.SetModelData(vertices);
	model.SetIndexData(indices);

	model.ResetAttributes();
	model.SetAttribute(0, gfx::AttributeType::Vec3);
	model.UploadAttributes();

	return true;
}