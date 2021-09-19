/* VERTEX SHADER */
#shader vert

#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in float aTexIndex;

out vec4 v_color;
out vec2 v_TexCoord;
out float v_TexIndex;

uniform mat4 u_cam;
uniform mat4 u_proj;

void main()
{
    v_color = aColor;
    v_TexCoord = aTexCoord;
    v_TexIndex = aTexIndex;
    vec3 myaPos = aPos;
    myaPos.x -= 0.0;
    vec4 mult_pos = u_proj * u_cam * vec4( myaPos , 1.0);
    

    gl_Position = mult_pos;
}



/* FRAGMENT SHADER */
#shader frag


#version 330 core

layout(location = 0) out vec4 color;
in vec4 v_color;
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Textures[32];

void main()
{
    int index = int(v_TexIndex);
    //color = texture(u_Textures[index], v_TexCoord);
    
    for (int k = 0; k < 32; ++k)
        if (index == k)
            color = /* texture(u_Textures[k], v_TexCoord) * */ v_color;

    //color += vec4( v_TexCoord*0.2, 0.0, 0.3);
    if ( color.a <= 0.1f )
        discard;
}