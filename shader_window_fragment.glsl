#version 330 core
precision highp float;

in vec2 window_coord;
in float geometry_size;

uniform sampler2D window_sampler;
uniform sampler2D icon_sampler;
uniform int picking_mode;
uniform float window_id;

out vec4 fragColor;

void main() {
  if (picking_mode == 1) {
    fragColor = vec4(window_coord.x, window_coord.y, window_id, 1.);
  } else {
    if (geometry_size > .4) {
      fragColor = texture(window_sampler, window_coord).rgba;
    } else {
      fragColor = texture(icon_sampler, window_coord).rgba;
    }
  }
}
