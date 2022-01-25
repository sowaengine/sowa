#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 texCoords;

void main()
{
   gl_Position = vec4(aPos, 1.0);
   texCoords = aTexCoords;
}


#shader fragment
#version 330 core

out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D tex;

const float offset_x = 1.0 / 1280.0;
const float offset_y = 1.0 / 720.0;

vec2 offsets[9] = vec2[]
(
   vec2(-offset_x, offset_y),     vec2(0.0, offset_y),     vec2(offset_x, offset_y),
   vec2(-offset_x, 0.0)    ,      vec2(0.0, 0.0)     ,     vec2(offset_x, 0.0),
   vec2(-offset_x, -offset_y),    vec2(0.0, -offset_y),    vec2(offset_x, -offset_y)
);


float edge_kernel[9] = float[]
(
   1,  2,  3,
   -4,  1,  4,
   -3,  -2,  -1
);
float kernel[9] = float[]
(
    0,  0,  0,
    0,  1,  0,
    0,  0,  0
);

void main()
{
   FragColor = texture(tex, texCoords);

   vec3 color = vec3(0.0f);
   for(int i = 0; i<9; i++)
      color += vec3(texture(tex, texCoords.st + offsets[i])) * kernel[i];
   FragColor = vec4(color, 1.0);
}