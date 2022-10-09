#shader vertex
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 uModel;
uniform mat4 uViewProjection;

out vec2 vTexCoords;

void main()
{
    gl_Position = uViewProjection * uModel * vec4(aPos.x, aPos.y, 0.0, 1.0);
    vTexCoords = aTexCoords;
}


#shader fragment
#version 330 core

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out int gDrawID;


in vec2 vTexCoords;

uniform sampler2D uTexture;
uniform vec3 uTint;
uniform int uDrawID;

void main()
{
	gAlbedo = texture(uTexture, vTexCoords) * vec4(uTint, 1.0);
	gDrawID = uDrawID;

    if(gAlbedo.a < 0.1)
        discard;
}