#version 330 core

layout(location = 0) out vec4 color;
layout(location = 1) out int drawId;

in vec2 vTexCoords;

uniform sampler2D uAlbedo;
uniform isampler2D uDrawId;

void main() {
    vec4 tex = texture(uAlbedo, vTexCoords);
    color = tex;

    int id = texture(uDrawId, vTexCoords).r;
    drawId = id;

    if(tex.a < 0.5)
        discard;
}