#shader vertex
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aTexture;
layout(location = 4) in uint aEntityToken;
layout(location = 5) in vec4 aClip;

uniform mat4 uProj;
uniform mat4 uView;

out vec4 vColor;
out vec2 vUV;
out float vTexture;
flat out uint vEntityToken;
out vec2 vPosition;
flat out vec4 vClip;

void main() {
	gl_Position = uProj * uView * vec4(aPos, 1.0);
	vColor = aColor;
	vUV = aUV;
	vTexture = aTexture;
	vEntityToken = aEntityToken;
	vPosition = aPos.xy;
	vClip = aClip;
}

#shader fragment
#version 330 core

in vec4 vColor;
in vec2 vUV;
in float vTexture;
flat in uint vEntityToken;
in vec2 vPosition;
flat in vec4 vClip;

uniform sampler2D uTextures[16];
uniform float uAlphaCutoff;

layout(location = 0) out vec4 color;
layout(location = 1) out uint entityToken;

vec4 sampleTexture(int index) {
	if (index == 0) return texture(uTextures[0], vUV);
	else if (index == 1) return texture(uTextures[1], vUV);
	else if (index == 2) return texture(uTextures[2], vUV);
	else if (index == 3) return texture(uTextures[3], vUV);
	else if (index == 4) return texture(uTextures[4], vUV);
	else if (index == 5) return texture(uTextures[5], vUV);
	else if (index == 6) return texture(uTextures[6], vUV);
	else if (index == 7) return texture(uTextures[7], vUV);
	else if (index == 8) return texture(uTextures[8], vUV);
	else if (index == 9) return texture(uTextures[9], vUV);
	else if (index == 10) return texture(uTextures[10], vUV);
	else if (index == 11) return texture(uTextures[11], vUV);
	else if (index == 12) return texture(uTextures[12], vUV);
	else if (index == 13) return texture(uTextures[13], vUV);
	else if (index == 14) return texture(uTextures[14], vUV);
	return texture(uTextures[15], vUV);
}

void main() {
	if (vPosition.x < vClip.x || vPosition.y < vClip.y ||
		vPosition.x >= vClip.z || vPosition.y >= vClip.w) discard;
	color = sampleTexture(int(vTexture)) * vColor;
	if (color.a < uAlphaCutoff)
		discard;
	entityToken = vEntityToken;
}
