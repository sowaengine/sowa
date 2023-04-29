#include "shader_gl.hpp"

#include "gfx/gl/glfuncs.hpp"

namespace sowa
{
namespace gfx
{

GLShader::~GLShader() {
    Delete();
}

void GLShader::SetVertexSource(const std::string& src) {
    if(m_id != 0)
        return;

    m_vertexSource = src;
}
void GLShader::SetFragmentSource(const std::string& src) {
    if(m_id != 0)
        return;
    
    m_fragmentSource = src;
}

void GLShader::Build() {
    if(m_id != 0) {
        Delete();
    }

    uint32_t vertexShader = GL().createShader(GLShaderType::Vertex);
    uint32_t fragmentShader = GL().createShader(GLShaderType::Fragment);
    GL().shaderSource(vertexShader, m_vertexSource);
    GL().compileShader(vertexShader, GLShaderType::Vertex);

    GL().shaderSource(fragmentShader, m_fragmentSource);
    GL().compileShader(fragmentShader, GLShaderType::Fragment);

    m_id = GL().createProgram();
    GL().attachShader(m_id, vertexShader);
    GL().attachShader(m_id, fragmentShader);
    GL().linkProgram(m_id);
}

void GLShader::New(const std::string &vsource, const std::string& fsource) {
    SetVertexSource(vsource);
    SetFragmentSource(fsource);
    Build();
}

void GLShader::Delete() {
    if(m_id != 0) {
        GL().deleteProgram(m_id);
    }
}

void GLShader::Bind() {
    GL().useProgram(m_id);
}
void GLShader::Unbind() {
    GL().useProgram(0);
}

} // namespace gfx
} // namespace sowa
