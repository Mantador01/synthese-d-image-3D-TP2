//! \file fragment.glsl

#version 430

#ifdef VERTEX_SHADER
uniform mat4 mvpMatrix;

layout(location= 0) in vec3 position;
layout(location= 1) in vec3 color;

out vec3 vertex_color;
void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    vertex_color= color;
}
#endif


#ifdef FRAGMENT_SHADER

in vec3 vertex_color;

out vec4 fragment_color;
void main( )
{
    //~ float color= 0.3*vertex_color.r + 0.8*vertex_color.g + 0.1*vertex_color.b;
    //~ fragment_color= vec4(vec3(color), 1);
    float color= 0.3*vertex_color.r + 0.8*vertex_color.g + 0.1*vertex_color.b;
    fragment_color= vec4(vertex_color, 1);
}
#endif
