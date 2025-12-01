
#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 2) in vec3 normal;
layout(location= 4) in uint material;

uniform mat4 mvpMatrix;
uniform mat4 mvMatrix;

out vec3 vertex_position;
out vec3 vertex_normal;
flat out uint vertex_material;
/* decoration flat : le varying est un entier, donc pas vraiment interpolable... il faut le declarer explicitement */

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    
    // position et normale dans le repere camera
    vertex_position= vec3(mvMatrix * vec4(position, 1));
    vertex_normal= mat3(mvMatrix) * normal;
    
    // et transmet aussi l'indice de la matiere au fragment shader...
    vertex_material= material;
}

#endif


#ifdef FRAGMENT_SHADER
out vec4 fragment_color;

in vec3 vertex_position;
in vec3 vertex_normal;
flat in uint vertex_material;	// !! decoration flat, le varying est marque explicitement comme non interpolable  !!

const float PI= 3.141592;

#define MAX_MATERIALS 64
uniform vec4 diffuse_colors[MAX_MATERIALS];
uniform vec4 specular_colors[MAX_MATERIALS];
uniform float ns[MAX_MATERIALS];

uniform vec3 up;

void main( )
{
    vec3 o= normalize(-vertex_position);        // p o
    vec3 l= normalize(-vertex_position);        // p light.
    vec3 n= normalize(vertex_normal);
    //~ float cos_theta= max(0, dot(n, l));
    float cos_theta= abs(dot(n, l));
    
    vec3 h= normalize(l+o);
    float cos_theta_h= max(0, dot(n, h));
    vec3 blinn= (ns[vertex_material] + 8) / (8*PI) * specular_colors[vertex_material].rgb * pow(cos_theta_h, ns[vertex_material]);
    
    float ao= (dot(n, up)+1)/2;
    
    // recupere la couleur de la matiere du triangle, en fonction de son indice.
    vec3 color= diffuse_colors[vertex_material].rgb;
    fragment_color= vec4(ao / 4 * color / 2 + (color + blinn) * cos_theta, 1);
}

#endif
