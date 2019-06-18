#version 330 core
precision highp float;

in vec2 UV;
in float geometry_size;

uniform sampler2D windowSampler;
uniform sampler2D iconSampler;
uniform int pickingMode;
uniform float windowId;

out vec4 fragColor;

void main() {
  if (pickingMode == 1) {
    fragColor = vec4(UV.x, UV.y, windowId, 1.);
  } else {
    if (geometry_size > .4) {
      fragColor = texture(windowSampler, UV).rgba;
    } else {
      fragColor = texture(iconSampler, UV).rgba;
    }
  }
}
