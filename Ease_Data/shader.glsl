#shader vertex

#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aColor;

out vec2 texCoords;
out vec3 color;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

void main()
{
   gl_Position =  u_Proj * u_View * u_Model * vec4(aPos, 1.0);
   texCoords = aTexCoords;
   color = aColor;
}


#shader fragment
#version 330 core

layout(location = 0) out vec4 FragColor;

in vec2 texCoords;
in vec3 color;

uniform sampler2D tex;
uniform int outline;
uniform int entityID;

void main()
{
   FragColor = mix( texture(tex, texCoords), vec4(color, 1.0), 0.0);
   if(outline == 1)
      FragColor = vec4(0.1f, 0.5f, 0.8f, 1.0f);

   if(FragColor.a <= 0.1)
      discard;
}