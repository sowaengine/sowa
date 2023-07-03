#version 300 es
precision mediump float;

layout(location = 0) out vec4 FragColor;

in vec2 sUV;

void main() {
  FragColor = vec4(sUV, 0.0, 1.0);
}