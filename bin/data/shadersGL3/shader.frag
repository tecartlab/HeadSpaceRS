#version 150

// this is how we receive the texture
uniform sampler2DRect tex0;

uniform float lowerLimit;
uniform float upperLimit;
uniform int mask;

in vec3 worldPos;

out vec4 outputColor;

void main()
{
    float height =  (worldPos.z - lowerLimit) / (upperLimit - lowerLimit);
    if(mask == 1){
      outputColor = vec4(0,0,0,height * 10000);
    } else {
      outputColor = vec4(height,height,height,1);
    }
}
