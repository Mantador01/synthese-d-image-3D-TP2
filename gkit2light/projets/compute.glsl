
#version 430

#ifdef COMPUTE_SHADER

layout(std430, binding= 0) readonly buffer entreeBuffer
{
	int entree[];
};

layout(std430, binding= 1) writeonly buffer sortieBuffer
{
	int sortie[];
};

layout(local_size_x= 256) in;
void main( )
{
	uint id= gl_GlobalInvocationID.x;
	if(id < entree.length())
		sortie[id] = entree[id] + 10;
}

#endif
