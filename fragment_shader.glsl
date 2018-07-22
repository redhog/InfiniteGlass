#version 130 // Specify which version of GLSL we are using.

precision highp float; // Video card drivers require this line to function properly

// Interpolated values from the vertex shaders
in vec2 UV;

uniform sampler2D myTextureSampler;

out vec4 fragColor;

void main() {
  fragColor = texture(myTextureSampler, UV).rgba;
}


