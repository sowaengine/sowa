#version 330 core

out vec4 FragColor;

in vec2 vTexCoords;

uniform sampler2D gAlbedo;

void main() {
    vec4 tex = texture(gAlbedo, vTexCoords);
    FragColor = tex;

    if(tex.a < 0.5)
        // FragColor = vec4(vTexCoords.x, 0, vTexCoords.y, 1.0);
        discard;
}