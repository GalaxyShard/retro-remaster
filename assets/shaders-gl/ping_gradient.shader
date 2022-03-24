#shader vertex
#version 330 core

layout(location = 0) in vec4 pos;
uniform mat4 u_mvp;

out vec4 v_pos;
void main()
{
    gl_Position = u_mvp * pos;
    //gl_Position = vec4(pos * u_scale + u_pos, 1);
    v_pos = gl_Position;
}

#shader fragment
#version 330 core

out vec4 color;
in vec4 v_pos;
void main()
{
    vec4 squared = v_pos*v_pos;
    float dist = sqrt(squared.x + squared.y + squared.z) * 0.5;
    color = vec4(dist, 0.25, 0.5, 1);
}