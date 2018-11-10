#version 120

uniform mat4 viewMatrixInverse;

varying vec3 worldPos;

void main()
{
    // send the vertices to the fragment shader
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    worldPos = vec3((viewMatrixInverse * gl_ModelViewMatrix * gl_Vertex).xyz);
}
