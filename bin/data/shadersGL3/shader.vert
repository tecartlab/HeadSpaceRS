#version 150

// these are for the programmable pipeline system and are passed in
// by default from OpenFrameworks
uniform mat4 modelViewProjectionMatrix;

// from the app
uniform mat4 viewMatrixInverse;

in vec4 position;
in vec2 texcoord;

// this is something we're creating for this shader
out vec3 worldPos;

void main()
{
    // send the vertices to the fragment shader
    gl_Position = modelViewProjectionMatrix * position;

    worldPos = vec3((viewMatrixInverse * gl_ModelViewMatrix * position).xyz);
}
