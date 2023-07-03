#version 300 es
precision mediump float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out vec2 sUV;
void main() {
  sUV = aUV;

  gl_Position = vec4(aPos.x - 1.0, -(aPos.y - 1.0), aPos.z, 1.0);
}