#version 120

// this is coming from our C++ code
uniform float lowerLimit;
uniform float upperLimit;

varying vec3 worldPos;
varying vec2 texcoord;

void main()
{
    float height =  (worldPos.z - lowerLimit) / (upperLimit - lowerLimit);
    vec4 color = vec4(height,height,height,1);
    gl_FragColor = color;
}
