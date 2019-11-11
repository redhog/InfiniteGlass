#version 330 core
precision highp float;

#define ICON_CUTOFF_1 .5
#define ICON_CUTOFF_2 .3

in vec2 window_coord;
in float geometry_size;

uniform sampler2D window_sampler;
uniform sampler2D WM_HINTS_icon;
uniform sampler2D WM_HINTS_icon_mask;
uniform int WM_HINTS_icon_enabled;
uniform int WM_HINTS_icon_mask_enabled;
uniform int picking_mode;
uniform float window_id;
uniform sampler2D IG_CONTENT;
uniform vec4 IG_CONTENT_transform;

uniform int ATOM_IG_LAYER_MENU;
uniform int IG_LAYER;

out vec4 fragColor;


vec4 icon_color;
vec4 window_color;

float scale;

void main() {
  if (picking_mode == 1) {
    fragColor = vec4(window_coord.x, window_coord.y, window_id, 1.);
  } else {
    if (window_coord.x < 0. || window_coord.x > 1. || window_coord.y < 0. || window_coord.y > 1.) {
      fragColor = vec4(0., 0., 1., 1.);
    } else {
      if (!isnan(IG_CONTENT_transform[0])) {
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
        if (WM_HINTS_icon_enabled == 1) {
          icon_color = texture(WM_HINTS_icon, window_coord).rgba;
        } else {
          icon_color =  vec4(1., 0., 0., 1.);
        }
        if (WM_HINTS_icon_mask_enabled == 1) {
          icon_color.a = 1. - texture(WM_HINTS_icon_mask, window_coord).r;
        }

        if (IG_LAYER == ATOM_IG_LAYER_MENU || geometry_size > ICON_CUTOFF_1) {
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
}
