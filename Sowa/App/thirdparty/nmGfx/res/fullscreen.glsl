#shader vertex
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 uModel;
uniform mat4 uViewProjection;

out vec2 vTexCoords;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
    vTexCoords = aTexCoords;
}


#shader fragment
#version 330 core

out vec4 FragColor;


in vec2 vTexCoords;

uniform sampler2D gAlbedo;
uniform sampler2D gPosition;
uniform sampler2D gNormal;

void main()
{
    /* GBuffer debug drawing
    {
        float sector = 1280 / 3;

        if(gl_FragCoord.x > sector * 0)
        {
            FragColor = vec4(texture(gAlbedo, vTexCoords).rgb, 1.0);
        }
        if(gl_FragCoord.x > sector * 1)
        {
            FragColor = texture(gPosition, vTexCoords);
        }
        if(gl_FragCoord.x > sector * 2)
        {
            FragColor = texture(gNormal, vTexCoords);
        }
    }
    */

    FragColor = texture(gAlbedo, vTexCoords);
    if(FragColor.a < 0.1)
        discard;
}