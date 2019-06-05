#version 330 core
precision highp float;

in vec2 UV;

uniform sampler2D myTextureSampler;
uniform int pickingMode;
uniform float windowId;

out vec4 fragColor;

void main() {
  if (pickingMode == 1) {
    fragColor = vec4(UV.x, UV.y, windowId, 1.);
  } else {
    fragColor = texture(myTextureSampler, UV).rgba;
  }
}
