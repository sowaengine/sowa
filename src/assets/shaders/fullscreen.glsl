#shader vertex
#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 vUV;

void main() {
	gl_Position = vec4(aPos, 0.0, 1.0);
	vUV = aUV;
}

#shader fragment
#version 330 core

in vec2 vUV;

uniform sampler2D uTexture;

out vec4 color;

void main() {
	color = texture(uTexture, vUV);
}
