#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
//uniform mat4 u_MVP;

out vec3 color;
out vec2 texCoords;
void main()
{
  gl_Position = proj * view * model * vec4(aPos, 1.0f);

  color = aColor;
  texCoords = aTexCoords;
}


#shader fragment
#version 330 core
out vec4 FragColor;

in vec3 color;
in vec2 texCoords;

uniform sampler2D u_Texture;
void main()
{
   FragColor = texture(u_Texture, texCoords);
   //FragColor = vec4(texCoords, 1.0f, 1.0f);
   if(FragColor.a < 0.1)
      discard;
}