#version 420

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in float texIndex;


layout(location = 0) out vec4 outColor;


//uniform sampler2D u_Textures[32];



void main() {
   //int index = int(texIndex);

   outColor = vec4(1.0, 1.0, 1.0, 1.0);
   //outColor = texture(u_Textures[index], fragTexCoord);



   if(outColor.a < 0.1)
      discard;
}
