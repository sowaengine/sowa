#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 uModel;
uniform mat4 uViewProjection;


out vec2 vTexCoords;
out vec3 vNormal;
out vec3 vModelPos;

void main()
{
    vModelPos = vec3(uModel * vec4(aPos, 1.0f));

    gl_Position = uViewProjection * uModel * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    

    vTexCoords = aTexCoords;
    vNormal = aNormal;
}


#shader fragment
#version 330 core

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec4 gPosition;
layout(location = 2) out vec4 gNormal;
layout(location = 3) out int gDrawID;


in vec2 vTexCoords;
in vec3 vNormal;
in vec3 vModelPos;


uniform int uDrawID;
// Material
uniform vec4 uMat_Albedo;
uniform sampler2D uMat_AlbedoTex;
uniform float uMat_Specular;
uniform sampler2D uMat_SpecularTex;


void main()
{
	vec4 albedo_tex = texture(uMat_AlbedoTex, vTexCoords).rgba;
	float specular_tex = texture(uMat_SpecularTex, vTexCoords).r;
	
	gAlbedo = albedo_tex * uMat_Albedo;
	
	gPosition = vec4(vModelPos, 1.0);
	gNormal = vec4(normalize(vNormal), 1.0);

	gDrawID = uDrawID;
}