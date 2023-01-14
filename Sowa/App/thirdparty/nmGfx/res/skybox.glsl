#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;

out vec3 vTexCoords;

uniform mat4 uViewProj;

void main()
{
    vec4 pos = uViewProj * vec4(aPos, 1.0);
    gl_Position = vec4(pos.x, pos.y, pos.w, pos.w);

    vTexCoords = vec3(aPos.x, aPos.y, -aPos.z);
}

#shader fragment
#version 330 core

out vec4 FragColor;

in vec3 vTexCoords;
uniform samplerCube uSkybox;

void main()
{
    FragColor = texture(uSkybox, vTexCoords);
}