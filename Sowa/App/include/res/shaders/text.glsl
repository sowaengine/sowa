#shader vertex

#version 330 core
layout (location = 0) in vec4 aPosUv;
out vec2 vTexCoords;

uniform mat4 projection;
uniform mat4 model;

void main()
{
    gl_Position = projection * model * vec4(aPosUv.xy, 0.0, 1.0);
    vTexCoords = aPosUv.zw;
}  

#shader fragment

#version 330 core
in vec2 vTexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, vTexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
    if(color.a < 0.1) {
        discard;
    }
}  