#version 300 es
precision mediump float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aDrawId;
layout(location = 4) in float aTexture;
layout(location = 5) in float aDrawMode;

uniform mat4 uProj;
uniform mat4 uView;

out vec4 vColor;
out vec2 vTexCoords;
out float vDrawId;
out float vTexture;
out float vDrawMode;

void main() {
  gl_Position = uProj * uView * vec4(aPos.x, aPos.y, aPos.z, 1.0f);

  vColor = aColor;
  vTexCoords = aTexCoords;
  vDrawId = aDrawId;
  vTexture = aTexture;
  vDrawMode = aDrawMode;
}