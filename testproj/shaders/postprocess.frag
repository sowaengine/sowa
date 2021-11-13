#version 420

layout(location = 0) in vec2 fragTexCoord;


layout(location = 0) out vec4 outColor;


uniform vec4 u_Color;
uniform sampler2D u_AlbedoTexture;




void main() {
   //vec4 color = texture(u_AlbedoTexture, fragTexCoord) * u_Color;
   
   //float avg = (color.r + color.g + color.b) / 3.0f;
   //outColor = vec4(avg, avg, avg, color.a);


   // directly returns without post processing
   outColor = texture(u_AlbedoTexture, fragTexCoord);
   
    
    // depth value = gl_FragCoord.z
    //outColor = vec4(vec3(gl_FragCoord.z), 1.0);
    
    if(outColor.a < 0.1)
        discard;
}
