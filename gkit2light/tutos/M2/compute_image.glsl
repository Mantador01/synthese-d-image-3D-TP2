
//! \file compute_image.glsl compute shader + images

#version 430

#ifdef COMPUTE_SHADER

layout(binding= 0, rgba32f)  readonly uniform image2D a;
layout(binding= 1, rgba32f)  writeonly uniform image2D b;

layout(local_size_x= 8, local_size_y= 8) in;
void main( )
{
    vec4 pixel= imageLoad(a, ivec2(gl_GlobalInvocationID.xy));
    pixel= pixel / vec4(2, 2, 2, 1);    // ne modifie pas alpha...
    
    imageStore(b, ivec2(gl_GlobalInvocationID.xy), pixel);
}

#endif
