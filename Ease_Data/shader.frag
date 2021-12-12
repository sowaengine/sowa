#version 330 core
out vec3 color;

// Interpolated values from the vertex shaders
in vec3 fragmentColor;

void main(){
   // Output color = color specified in the vertex shader,
   // interpolated between all 3 surrounding vertices
   color = fragmentColor;
   
   // color = vec3(1,0,0);
}
