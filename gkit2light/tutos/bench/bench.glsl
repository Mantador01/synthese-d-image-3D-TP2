
#version 450

#extension GL_ARB_shader_ballot : require
#extension GL_ARB_gpu_shader_int64 : require


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

layout(binding= 0, r32ui) coherent uniform uimage2D image;
layout(binding= 0) buffer triangleData
{
	uint triangles[];
};

in vec3 vertex_position;
out vec4 fragment_color;


int bitCount64( const in uint64_t mask )
{
	return bitCount(unpackUint2x32(mask).y) + bitCount(unpackUint2x32(mask).x);
}

int findLSB64( const in uint64_t mask )
{
	return unpackUint2x32(mask).x != 0 ? findLSB(unpackUint2x32(mask).x) : 32 + findLSB(unpackUint2x32(mask).y);
}


layout(early_fragment_tests) in;
void main( )
{
	atomicAdd(triangles[gl_PrimitiveID], 1);
	imageAtomicAdd(image, ivec2(gl_FragCoord.xy), 1);
	
	//~ uint64_t mask= ballotARB(true);
	uint64_t mask= ballotARB(gl_HelperInvocation == false);
	float v= float(bitCount64(mask)) / float(gl_SubGroupSizeARB);
	fragment_color= vec4(1 - v, v, 0, 1);
	if(v > 1)
		fragment_color= vec4(1, 1, 1, 1);
}
#endif
