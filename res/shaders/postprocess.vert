#version 420

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoords;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;



layout(location = 0) out vec2 texCoords;

void main() {
    gl_Position = u_Proj * u_View * u_Model * vec4(inPosition, 1.0);
    //gl_Position = vec4(inPosition, 1.0);
    //fragColor = vec4(0.0, 1.0, 1.0, 1.0); //inColor;
    //fragColor = u_Color;
    texCoords = inTexCoords;
}
