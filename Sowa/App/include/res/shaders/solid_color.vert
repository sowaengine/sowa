#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

uniform mat4 uModel;
uniform mat4 uProj;
out vec2 vTexCoords;

void main() {
    gl_Position = uProj * uModel * vec4(aPos.x, aPos.y, 0.0, 1.0);
    vTexCoords = aTexCoords;
}