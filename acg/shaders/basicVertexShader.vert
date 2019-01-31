#version 400 compatibility
layout(location = 0) in vec4 particle;

uniform mat4 modelViewProjectionMatrix;

out vec3 viewSpacePosition;

void main()
{


    // Transform position.
    gl_Position  = modelViewProjectionMatrix * vec4(particle.xyz, 1);

}