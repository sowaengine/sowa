#version 300 es
precision mediump float;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int ID;

in vec2 sUV;
in vec4 sCOLOR;
in float sID;

void main() {
  FragColor = sCOLOR;
  if(FragColor.a < 0.1) {
    discard;
  }

  ID = int(sID);
}