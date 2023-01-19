#include "nm_Model.hpp"
#include <vector>
#include "glad/glad.h"
#include "tiny_obj_loader.h"

namespace nmGfx
{
    void Model::LoadFromFile(const char* path)
    {
        Create();

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path, nullptr);

        if(!err.empty())
        {
#ifdef NMGFX_PRINT_MESSAGES
            printf("Error Loading model: %s : %s\n", path, err.c_str());
            // return;
#endif
        }

        // layout -> vec3 pos, vec3 normal, vec2 uv
        std::vector<float> vertexData;
        std::vector<unsigned int> indexData;
        bool hasNormals = false;
        bool hasTexCoords = false;

        for (size_t s = 0; s < shapes.size(); s++)
        {
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                for (size_t v = 0; v < fv; v++)
                {
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    indexData.push_back(indexData.size());

                    tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                    tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                    tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];

                    vertexData.push_back(vx);
                    vertexData.push_back(vy);
                    vertexData.push_back(vz);

                    // Check if `normal_index` is zero or positive. negative = no normal data
                    if (idx.normal_index >= 0)
                    {
                        tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
                        tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
                        tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];

                        vertexData.push_back(nx);
                        vertexData.push_back(ny);
                        vertexData.push_back(nz);

                        hasNormals = true;
                    }
                    else
                    {
                        vertexData.push_back(0.f);
                        vertexData.push_back(0.f);
                        vertexData.push_back(0.f);
                    }

                    // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                    if (idx.texcoord_index >= 0)
                    {
                        tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                        tinyobj::real_t ty = 1.0f - attrib.texcoords[2*size_t(idx.texcoord_index)+1];

                        vertexData.push_back(tx);
                        vertexData.push_back(ty);

                        hasTexCoords = true;
                    }
                    else
                    {
                        vertexData.push_back(0.f);
                        vertexData.push_back(0.f);
                    }

                    // Optional: vertex colors
                    // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                    // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                    // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
                }
                index_offset += fv;

                // per-face material
                shapes[s].mesh.material_ids[f];
            }
        }

        ResetAttributes();
        SetModelData(vertexData);
        SetIndexData(indexData);
        SetAttribute(0, AttributeType::VEC3);
        SetAttribute(1, AttributeType::VEC3);
        SetAttribute(2, AttributeType::VEC2);
        UploadAttributes();

#ifdef NMGFX_PRINT_MESSAGES
        printf("Loaded Model %s, Vertex Count: %i\n",path, (int)(vertexData.size() / 8));
#endif

        return;
    };


    void Model::Create()
    {
        _vao2d.Create();
        _vbo2d.Create(BufferType::VERTEX_BUFFER);
        _ebo2d.Create(BufferType::INDEX_BUFFER);
    }

    void Model::SetIndexData(const std::vector<uint32_t>& data)
    {
        _indexCount = data.size();

        _vao2d.Use();
        _ebo2d.Use();
        _ebo2d.BufferData(data.data(), data.size() * sizeof(uint32_t), BufferUsage::STATIC_DRAW);
        VertexArray::Unbind();
    }
    void Model::SetModelData(const std::vector<float>& data)
    {
        _vbodata_size = data.size();
        _vao2d.Use();
        _vbo2d.Use();
        _vbo2d.BufferData(data.data(), data.size() * sizeof(float), BufferUsage::STATIC_DRAW);
        VertexArray::Unbind();
    }
    void Model::ResetAttributes()
    {
        _vao2d.Use();
        _vao2d.ResetAttributes();
        _attributes.clear();
        VertexArray::Unbind();
    }
    void Model::SetAttribute(uint32_t slot, AttributeType type)
    {
        _vao2d.SetAttribute(slot, type);
    }
    void Model::UploadAttributes()
    {
        _vao2d.Use();
        _vbo2d.Use();
        if(_indexCount > 0)
            _ebo2d.Use();
        _vao2d.UploadAttributes();
    }


    void Model::Draw() const
    {
        _vao2d.Use();
        if(_indexCount > 0)
            glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
        else if(_vao2d._attributeSizeInBytes > 0)
            glDrawArrays(GL_TRIANGLES, 0, _vbodata_size / _vao2d._attributeSizeInBytes);
        VertexArray::Unbind();
    }
} // namespace nmGfx
