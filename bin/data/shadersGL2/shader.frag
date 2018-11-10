#version 120

// this is coming from our C++ code
uniform float lowerLimit;
uniform float upperLimit;
uniform int mask;

varying vec3 worldPos;

void main()
{
    float height =  (worldPos.z - lowerLimit) / (upperLimit - lowerLimit);
    if(mask == 1){
      gl_FragColor = vec4(0,0,0,height * 10000);
    } else {
      gl_FragColor = vec4(height,height,height,1);
    }
}
