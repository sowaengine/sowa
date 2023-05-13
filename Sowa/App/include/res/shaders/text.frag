#version 330 core
in vec2 vTexCoords;

layout(location = 0) out vec4 color;
layout(location = 1) out int gDrawID;

uniform sampler2D uTexture;
// uniform vec3 textColor;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(uTexture, vTexCoords).r);
    color = vec4(1.0, 1.0, 1.0, 1.0) * sampled;
    
    if(color.a < 0.5) {
        discard;
    }
} 