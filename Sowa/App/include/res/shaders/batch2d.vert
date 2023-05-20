#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in float aTexture;

uniform mat4 uProj;
uniform mat4 uView;

out vec4 vColor;
out vec2 vTexCoords;
out float vTexture;

void main() {
    gl_Position = uProj * uView * vec4(aPos.x, aPos.y, aPos.z, 1.0);

    vColor = aColor;
    vTexCoords = aTexCoords;
    vTexture = aTexture;
}