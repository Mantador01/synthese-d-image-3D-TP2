#version 430

#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;
layout(location= 2) in vec3 normal;
out vec3 vertex_normal;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    
    vertex_normal= mat3(mvMatrix) * normal;
}
#endif


#ifdef FRAGMENT_SHADER

in vec3 vertex_normal;
out vec4 fragment_color;

void main( )
{
    vec3 normal= normalize( vertex_normal );
    
    // matiere diffuse...
    vec3 color= vec3(0.8, 0.8, 0.8);
    float cos_theta= max(0.0, normal.z);
    color= color * cos_theta;
    
    fragment_color= vec4(color, 1);
}

#endif
