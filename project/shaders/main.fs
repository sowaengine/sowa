#version 300 es
precision mediump float;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int ID;

in vec2 sUV;

uniform sampler2D uTexture;

void main() {
  FragColor = texture(uTexture, sUV);
  ID = 1;
}