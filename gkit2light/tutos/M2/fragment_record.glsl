//! \file fragment_record.glsl

#version 430

#ifdef VERTEX_SHADER
uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;

in vec3 position;
out vec3 vertex_position;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    vertex_position= position;
}
#endif


#ifdef FRAGMENT_SHADER

struct fragment
{
    float x, y, z;
    float r, g, b;
};

layout(std430, binding= 0) buffer fragments
{
    uint count;
    uint instance_count;
    uint vertex_base;
    uint instance_base;
    
    fragment array[];
};

layout(binding= 0, r32ui) coherent uniform uimage2D counters;

in vec3 vertex_position;
out vec4 fragment_color;

layout(early_fragment_tests) in;

void main( )
{
    vec3 t= normalize( dFdx(vertex_position) );
    vec3 b= normalize( dFdy(vertex_position) );
    vec3 normal= normalize( cross(t, b) );
    vec3 color= (normal + 1) / 2;
	float g= 0.3*color.r + 0.7*color.g + 0.1*color.b;
	
	uint n= imageAtomicAdd(counters, ivec2(gl_FragCoord.xy), 1) +1;
	
    // utilise une palette de couleur pour afficher la valeur du compteur
    const vec3 colors[10]= vec3[10](
        vec3(0,0,0), 
        vec3(12,17,115),
        vec3(28,121,255),
        vec3(31,255,255),
        vec3(130,255,17),
        vec3(255,255,14),
        vec3(255,112,22),
        vec3(251,0,20),
        vec3(113,1,14),
        vec3(113,1,14)
    );
    if(n < 10) color= g * colors[n] / vec3(255);
    else color= g * colors[9] / vec3(255);
	
	//~ fragment_color= vec4(color, 1);
	fragment_color= vec4(vec3(g), 1);
    
	//
    uint offset= atomicAdd(count, 1);
    
    array[offset].x= vertex_position.x;
    array[offset].y= vertex_position.y;
    array[offset].z= vertex_position.z;
    array[offset].r= color.r;
    array[offset].g= color.g;
    array[offset].b= color.b;
}
#endif
