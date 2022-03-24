#shader vertex
#version 100

attribute vec4 pos;
uniform mat4 u_mvp;

varying vec4 v_pos;
void main()
{
    gl_Position = u_mvp * pos;
    v_pos = gl_Position;
}

#shader fragment
#version 100

varying highp vec4 v_pos;
void main()
{
    highp vec4 squared = v_pos*v_pos;
    highp float dist = sqrt(squared.x + squared.y + squared.z) * 0.5;
    gl_FragColor = vec4(dist, 0.25, 0.5, 1);
}