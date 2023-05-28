#version 330 core

layout(location = 0) out vec4 color;

in vec2 vTexCoords;

uniform sampler2D uAlbedo;

void main() {
    vec4 tex = texture(uAlbedo, vTexCoords);
    color = tex;

    if(tex.a < 0.5)
        discard;
}