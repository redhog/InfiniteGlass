#version 330 core
precision highp float;

#define ICON_CUTOFF_1 .5
#define ICON_CUTOFF_2 .3

in vec2 window_coord;
in float geometry_size;

uniform sampler2D window_sampler;
uniform sampler2D icon_sampler;
uniform sampler2D icon_mask_sampler;
uniform int picking_mode;
uniform int has_icon;
uniform int has_icon_mask;
uniform float window_id;
uniform sampler2D IG_CONTENT;
uniform vec4 IG_CONTENT_transform;

out vec4 fragColor;


vec4 icon_color;
vec4 window_color;

float scale;

void main() {
  if (picking_mode == 1) {
    fragColor = vec4(window_coord.x, window_coord.y, window_id, 1.);
  } else {
    if (IG_CONTENT_transform[2] == -1.) {
      mat4 transform_mat = transpose(mat4(
        1./IG_CONTENT_transform[2], 0., 0., -IG_CONTENT_transform[0]/IG_CONTENT_transform[2],
        0., 1./IG_CONTENT_transform[3], 0., -IG_CONTENT_transform[1]/IG_CONTENT_transform[3],
        0., 0., 1., 0.,
        0., 0., 0., 1.
      ));
      vec4 texture_coord = transform_mat * vec4(window_coord, 0, 1.);
      fragColor = texture(IG_CONTENT, texture_coord.xy).rgba;
    } else {
      window_color = texture(window_sampler, window_coord).rgba;
      if (has_icon == 1) {
        icon_color = texture(icon_sampler, window_coord).rgba;
      } else {
        icon_color =  vec4(1., 0., 0., 1.);
      }
      if (has_icon_mask == 1) {
        icon_color.a = 1. - texture(icon_mask_sampler, window_coord).r;
      }

      if (geometry_size > ICON_CUTOFF_1) {
        fragColor = window_color;
      } else if (geometry_size > ICON_CUTOFF_2) {
        scale = (geometry_size - ICON_CUTOFF_2) / (ICON_CUTOFF_1 - ICON_CUTOFF_2);
        fragColor = scale * window_color + (1 - scale) * icon_color;
      } else {
        fragColor = icon_color;
      }
    }
  }
}
