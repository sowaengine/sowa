#version 300 es
precision mediump float;

layout(location = 0) out vec4 FragColor;

in vec2 sUV;
in vec4 sCOLOR;
in float sID;

void main() {
  FragColor = sCOLOR;
  if(FragColor.a < 0.1) {
    discard;
  }
}