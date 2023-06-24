#include "vertex_array.hxx"

#include <algorithm>

namespace gfx {
VertexArray::~VertexArray() {
	Delete();
}

void VertexArray::New() {
	Delete();

	glGenVertexArrays(1, &m_id);
}
void VertexArray::Delete() {
	if (m_id != 0) {
		glDeleteVertexArrays(1, &m_id);
	}

	m_id = 0;
}

void VertexArray::Bind() {
	glBindVertexArray(m_id);
}
void VertexArray::Unbind() {
	glBindVertexArray(0);
}

void VertexArray::ResetAttributes() {
	for (const Attribute &attribute : m_attributes) {
		glDisableVertexAttribArray(attribute.slot);
	}

	m_attributeSizeInBytes = 0;
	m_attributes.clear();
}

void VertexArray::SetAttribute(uint32_t slot, attribute_t type) {
	m_attributes.emplace_back(slot, type);
	m_attributeSizeInBytes += AttributeType::GetAttributeSize(type);
}

void VertexArray::UploadAttributes() {
	std::sort(m_attributes.begin(), m_attributes.end());

	uint64_t bytes = 0;
	for (const Attribute &attribute : m_attributes) {
		glVertexAttribPointer(
			attribute.slot,
			AttributeType::GetComponentCount(attribute.type),
			AttributeType::GetAttributeType(attribute.type),
			false,
			m_attributeSizeInBytes,
			(const void *)bytes);

		glEnableVertexAttribArray(attribute.slot);
		bytes += AttributeType::GetAttributeSize(attribute.type);
	}
}

void VertexArray::DisableAttributes() {
	for (const Attribute &attribute : m_attributes) {
		glDisableVertexAttribArray(attribute.slot);
	}
}

} // namespace gfx
