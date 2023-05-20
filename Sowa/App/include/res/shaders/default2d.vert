#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

uniform mat4 uModel;
uniform mat4 uProj;
uniform mat4 uView;
// uniform mat4 uViewProjection;
uniform vec2 uBaseScale;

out vec2 vTexCoords;

void main() {
    // gl_Position = uViewProjection * uModel * vec4(aPos.x * uBaseScale.x, aPos.y * uBaseScale.y, 0.0, 1.0);
    // gl_Position = vec4(aPos.xy, 0.0, 1.0);
    gl_Position = uProj * uView * uModel * vec4(aPos.x * uBaseScale.x, aPos.y * uBaseScale.y, 0.0, 1.0);
    vTexCoords = aTexCoords;
}