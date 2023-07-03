#version 300 es
precision mediump float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aCOLOR;
layout(location = 3) in float aID;

out vec2 sUV;
out vec4 sCOLOR;
out float sID;

void main() {
  sUV = aUV;
  sCOLOR = aCOLOR;
  sID = aID;

  gl_Position = vec4(aPos.x - 1.0, -(aPos.y - 1.0), aPos.z, 1.0);
}