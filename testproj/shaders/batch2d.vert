#version 420

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in float inTexIndex;

// vec2 texcoords[4] = {
//    vec2(1.0, 1.0),
//    vec2(1.0, 0.0),
//    vec2(0.0, 0.0),
//    vec2(0.0, 1.0)
// };

//uniform mat4 u_Model;
//uniform mat4 u_View;
uniform mat4 u_Proj;


layout(location = 0) out vec2 texCoords;
layout(location = 1) out float texIndex;

void main() {
   // gl_Position = vec4( texCoords[gl_VertexID].x, texCoords[gl_VertexID].y, 0.0, 1.0);
   
   gl_Position = u_Proj * /* u_View * u_Model */ vec4(inPosition, 1.0);

    

   texCoords = inTexCoords;
   texIndex = inTexIndex;
}
