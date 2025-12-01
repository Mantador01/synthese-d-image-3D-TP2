
#version 330

#ifdef VERTEX_SHADER

layout(location= 0) in vec3 position;
out vec3 vertex_position;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;

void main( )
{
	gl_Position= mvpMatrix * vec4(position, 1);
	vertex_position= vec3(mvMatrix * vec4(position, 1));
}
#endif


#ifdef FRAGMENT_SHADER

in vec3 vertex_position;
out vec4 fragment_color;

layout(early_fragment_tests) in;
void main( )
{
    vec3 n= normalize( cross(dFdx(vertex_position), dFdy(vertex_position)) );
    fragment_color= vec4(abs(n.zzz), 1);
    //~ fragment_color= vec4(1);
}
#endif
