#version 120

uniform mat4 viewMatrixInverse;

varying vec3 worldPos;
varying vec2 texcoord;

void main()
{

    vec2 texcoord = gl_MultiTexCoord0.xy;

    // send the vertices to the fragment shader
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    worldPos = vec3((viewMatrixInverse * gl_ModelViewMatrix * gl_Vertex).xyz);
}
