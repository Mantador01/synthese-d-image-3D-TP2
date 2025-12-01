
#version 330

#ifdef VERTEX_SHADER

uniform mat4 mvpMatrix;

layout(location= 0) in vec3 position;

void main( )
{
	gl_Position= mvpMatrix * vec4(position, 1);
	gl_Position.w= -1;
	//~ gl_Position.w= 0;
}

#endif
