#version 300 es
precision mediump float;

layout(location = 0) out vec4 FragColor;

in vec2 sUV;

uniform sampler2D uTexture;

void main() {
  FragColor = texture(uTexture, sUV);
}