
#version 450

#ifdef COMPUTE_SHADER

layout(binding= 0, std430) readonly buffer fragmentsData    // [triangle count]
{
    uint fragments[];
};

// ne pas utiliser vec3, cf alignement sur multiple de 16 octets...
struct point
{
    float x, y, z;
};

layout(binding= 1, std430) readonly buffer positionsData // [vertex count]
{
    point positions[];
};

layout(binding= 2, std430) coherent buffer filledData    // [vertex count]
{
    uint filled_count;
    point filled[];
};


layout(local_size_x= 1024) in;
void main( )
{
    uint ID= gl_GlobalInvocationID.x;
    if(ID >= fragments.length())
        return;
    
    if(fragments[ID] > 0)
    //~ if(fragments[ID] == 0)
    {
        // ne copie que les triangles rasterises...
        uint index= atomicAdd(filled_count, 1);
        
        filled[3*index]= positions[3*ID];
        filled[3*index+1]= positions[3*ID+1];
        filled[3*index+2]= positions[3*ID+2];
    }
}

#endif
