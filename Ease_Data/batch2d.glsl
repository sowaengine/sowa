#shader vertex

#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in float aTexID;

out vec2 texCoords;
out float texID;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

void main()
{
   gl_Position = vec4(aPos, 0.0, 1.0);
   texCoords = aTexCoords;
   texID = aTexID;
}


#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 texCoords;
in float texID;
uniform int outline;

uniform sampler2D u_Textures[32];

void main()
{
   FragColor = vec4(1.0, 1.0, 1.0, 1.0);
   switch(int(texID))
	{
		case  0: FragColor *= texture(u_Textures[ 0], texCoords); break;
		case  1: FragColor *= texture(u_Textures[ 1], texCoords); break;
		case  2: FragColor *= texture(u_Textures[ 2], texCoords); break;
		case  3: FragColor *= texture(u_Textures[ 3], texCoords); break;
		case  4: FragColor *= texture(u_Textures[ 4], texCoords); break;
		case  5: FragColor *= texture(u_Textures[ 5], texCoords); break;
		case  6: FragColor *= texture(u_Textures[ 6], texCoords); break;
		case  7: FragColor *= texture(u_Textures[ 7], texCoords); break;
		case  8: FragColor *= texture(u_Textures[ 8], texCoords); break;
		case  9: FragColor *= texture(u_Textures[ 9], texCoords); break;
		case 10: FragColor *= texture(u_Textures[10], texCoords); break;
		case 11: FragColor *= texture(u_Textures[11], texCoords); break;
		case 12: FragColor *= texture(u_Textures[12], texCoords); break;
		case 13: FragColor *= texture(u_Textures[13], texCoords); break;
		case 14: FragColor *= texture(u_Textures[14], texCoords); break;
		case 15: FragColor *= texture(u_Textures[15], texCoords); break;
		case 16: FragColor *= texture(u_Textures[16], texCoords); break;
		case 17: FragColor *= texture(u_Textures[17], texCoords); break;
		case 18: FragColor *= texture(u_Textures[18], texCoords); break;
		case 19: FragColor *= texture(u_Textures[19], texCoords); break;
		case 20: FragColor *= texture(u_Textures[20], texCoords); break;
		case 21: FragColor *= texture(u_Textures[21], texCoords); break;
		case 22: FragColor *= texture(u_Textures[22], texCoords); break;
		case 23: FragColor *= texture(u_Textures[23], texCoords); break;
		case 24: FragColor *= texture(u_Textures[24], texCoords); break;
		case 25: FragColor *= texture(u_Textures[25], texCoords); break;
		case 26: FragColor *= texture(u_Textures[26], texCoords); break;
		case 27: FragColor *= texture(u_Textures[27], texCoords); break;
		case 28: FragColor *= texture(u_Textures[28], texCoords); break;
		case 29: FragColor *= texture(u_Textures[29], texCoords); break;
		case 30: FragColor *= texture(u_Textures[30], texCoords); break;
		case 31: FragColor *= texture(u_Textures[31], texCoords); break;
	}

	if(outline == 1)
      FragColor = vec4(0.1f, 0.5f, 0.8f, 1.0f);
   if(FragColor.a <= 0.1)
      discard;
}