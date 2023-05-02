#version 330 core

layout(location = 0) out vec4 color;

in vec2 vTexCoords;

uniform sampler2D uTexture;
uniform vec4 uColor;

void main() {
    if(texture(uTexture, vTexCoords).a < 0.5)
        discard;
    
    color = uColor;
}