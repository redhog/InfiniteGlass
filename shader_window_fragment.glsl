#version 330 core
precision highp float;

in vec2 UV;

uniform sampler2D myTextureSampler;

out vec4 fragColor;

void main() {
  fragColor = texture(myTextureSampler, UV).rgba;
}
