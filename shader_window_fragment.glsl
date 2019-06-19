#version 330 core
precision highp float;

in vec2 window_coord;
in float geometry_size;

uniform sampler2D window_sampler;
uniform sampler2D icon_sampler;
uniform int picking_mode;
uniform float window_id;

out vec4 fragColor;

vec4 icon_color;
vec4 window_color;

void main() {
  if (picking_mode == 1) {
    fragColor = vec4(window_coord.x, window_coord.y, window_id, 1.);
  } else {
    window_color = texture(window_sampler, window_coord).rgba;
    icon_color = texture(icon_sampler, window_coord).rgba;

    if (geometry_size > .4) {
      fragColor = window_color;
    } else {
      fragColor = (geometry_size / .4) * window_color + (1 - geometry_size / .4) * icon_color;
    }
  }
}
