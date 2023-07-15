#version 300 es
precision mediump float;

layout(location = 0) out vec4 color;
layout(location = 1) out int drawId;

in vec4 vColor;
in vec2 vTexCoords;
in float vTexture;
in float vDrawId;

uniform sampler2D uTextures[32];

vec4 getTexture();

float lerp(float from, float to, float t) {
  return from + ((to - from) * t);
}

void main() {
  drawId = int(vDrawId);
  // color = getTexture() * vColor;

  // diagonal gradient
  float mult = (vTexCoords.x + vTexCoords.y) / 2.0f;
  color = getTexture() * vColor * vec4(mult, mult, mult, 1.0f);

  if(color.a < 0.1f)
    discard;
}

vec4 getTexture() {
  int index = int(vTexture);

  if(index == 0)
    return texture(uTextures[0], vTexCoords);
  else if(index == 1)
    return texture(uTextures[1], vTexCoords);
  else if(index == 2)
    return texture(uTextures[2], vTexCoords);
  else if(index == 3)
    return texture(uTextures[3], vTexCoords);
  else if(index == 4)
    return texture(uTextures[4], vTexCoords);
  else if(index == 5)
    return texture(uTextures[5], vTexCoords);
  else if(index == 6)
    return texture(uTextures[6], vTexCoords);
  else if(index == 7)
    return texture(uTextures[7], vTexCoords);
  else if(index == 8)
    return texture(uTextures[8], vTexCoords);
  else if(index == 9)
    return texture(uTextures[9], vTexCoords);
  else if(index == 10)
    return texture(uTextures[10], vTexCoords);
  else if(index == 11)
    return texture(uTextures[11], vTexCoords);
  else if(index == 12)
    return texture(uTextures[12], vTexCoords);
  else if(index == 13)
    return texture(uTextures[13], vTexCoords);
  else if(index == 14)
    return texture(uTextures[14], vTexCoords);
  else if(index == 15)
    return texture(uTextures[15], vTexCoords);
  else if(index == 16)
    return texture(uTextures[16], vTexCoords);
  else if(index == 17)
    return texture(uTextures[17], vTexCoords);
  else if(index == 18)
    return texture(uTextures[18], vTexCoords);
  else if(index == 19)
    return texture(uTextures[19], vTexCoords);
  else if(index == 20)
    return texture(uTextures[20], vTexCoords);
  else if(index == 21)
    return texture(uTextures[21], vTexCoords);
  else if(index == 22)
    return texture(uTextures[22], vTexCoords);
  else if(index == 23)
    return texture(uTextures[23], vTexCoords);
  else if(index == 24)
    return texture(uTextures[24], vTexCoords);
  else if(index == 25)
    return texture(uTextures[25], vTexCoords);
  else if(index == 26)
    return texture(uTextures[26], vTexCoords);
  else if(index == 27)
    return texture(uTextures[27], vTexCoords);
  else if(index == 28)
    return texture(uTextures[28], vTexCoords);
  else if(index == 29)
    return texture(uTextures[29], vTexCoords);
  else if(index == 30)
    return texture(uTextures[30], vTexCoords);
  else if(index == 31)
    return texture(uTextures[31], vTexCoords);
  return vec4(1.f, 1.f, 1.f, 1.f);
}