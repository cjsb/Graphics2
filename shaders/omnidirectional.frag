#version 430

in vec4 FragPos;

uniform vec3 lightPos;

float far = 1000;

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(FragPos.xyz - lightPos);
    
    // map to [0;1] range by dividing by far_plane
    lightDistance = lightDistance / far;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}  