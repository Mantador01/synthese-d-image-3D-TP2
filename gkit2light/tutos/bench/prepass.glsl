
#version 330

#ifdef VERTEX_SHADER
in vec3 position;

uniform mat4 mvpMatrix;

void main( )
{
	gl_Position= mvpMatrix * vec4(position, 1);
}
#endif

