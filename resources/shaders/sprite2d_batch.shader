#shader vert
#version 330 core

layout (location = 0) in vec4 a_Position;


void main()
{
   gl_Position = a_Position;
}



#shader frag
#version 330 core

layout (location = 0) out vec4 color;

void main()
{
   color = vec4(0.8f, 0.5f, 0.5f, 1.0f);
}