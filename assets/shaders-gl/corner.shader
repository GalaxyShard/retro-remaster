#shader vertex
#version 330 core

layout(location = 0) in vec4 pos;
layout(location = 1) in vec2 texCoord;

uniform mat4 u_mvp;
out vec2 v_texCoord;

void main()
{
    gl_Position = u_mvp * pos;
    v_texCoord = texCoord;
}

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D u_tex;
uniform vec4 u_color;
in vec2 v_texCoord;

//uniform vec2 u_dimensions;
uniform float u_radius;

void main()
{
    //vec2 coords = v_texCoord * u_dimensions;
    //if (length(v_texCoord - vec2(0, 0)) < u_radius ||
    //    length(v_texCoord - vec2(0, y)) < u_radius ||
    //    length(v_texCoord - vec2(x, 0)) < u_radius ||
    //    length(v_texCoord - vec2(x, y)) < u_radius) {
    //    discard;
    //}
    

    if (vec2(u_radius,u_radius))
    {
        discard;
    }
    color = texture(u_tex, v_texCoord) * u_color;
}