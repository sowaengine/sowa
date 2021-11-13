#version 420

layout(location = 0) in vec2 fragTexCoord;


layout(location = 0) out vec4 outColor;


uniform vec4 u_Color;
uniform sampler2D u_AlbedoTexture;



void main() {
    //outColor = vec4(fragColor, 1.0);
    //outColor = fragColor;
    // outColor = vec4(fragTexCoord, 0.0, 1.0);
    // outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);

    outColor = texture(u_AlbedoTexture, fragTexCoord) * u_Color;
    
    
    // depth value = gl_FragCoord.z
    //outColor = vec4(vec3(gl_FragCoord.z), 1.0);
    
    if(outColor.a < 0.1)
        discard;
}
