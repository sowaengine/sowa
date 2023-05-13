#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 vTexCoords;

uniform mat4 uProj;

void main()
{
    gl_Position = uProj * /** uModel*/ vec4(aPos.x, aPos.y, 0.0, 1.0);

    vTexCoords = aTexCoords;
}