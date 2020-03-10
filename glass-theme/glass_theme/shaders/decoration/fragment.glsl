#version 330 core
precision highp float;

#include "resource://glass_theme/shaders/lib/fragment_svg.glsl"
#include "resource://glass_theme/shaders/lib/fragment_content.glsl"
#include "resource://glass_theme/shaders/lib/fragment_picking.glsl"

in vec2 px_window_bottom_left;
in vec2 px_window_top_right;
in vec2 px_coord;

in vec2 px_parent_window_bottom_left;
in vec2 px_parent_window_top_right;

uniform ivec2 size;
uniform ivec2 pointer;
uniform int atom_IG_LAYER_DESKTOP;

uniform int atom_IG_DECORATION_SAVE;
uniform int atom_IG_DECORATION_GHOSTS_ENABLE;
uniform int IG_DECORATION;

uniform int atom_IG_GHOST;
uniform int parent_IG_GHOST;
uniform int parent_IG_GHOSTS_DISABLED;

out vec4 fragColor;

uniform sampler2D IG_CONTENT_ALT;
uniform vec4 IG_CONTENT_ALT_transform;

void main() {
  vec2 window_coord = px_coord - px_window_bottom_left;
  vec2 window_size = px_window_top_right - px_window_bottom_left;
  vec2 scaled_window_coord = vec2(window_coord.x / window_size.x, 1. - window_coord.y / window_size.y);

  vec2 parent_window_size = px_parent_window_top_right - px_parent_window_bottom_left;
  vec2 geometry_size = parent_window_size / size;
  float geometry_scale = sqrt(pow(geometry_size.x, 2) + pow(geometry_size.y, 2));

  vec2 center = (px_window_top_right + px_window_bottom_left) / 2;

  float mouse_dist = sqrt(pow(center.x - pointer.x, 2) + pow(center.y - pointer.y, 2));

  if (geometry_scale < 0.25 || mouse_dist > 150.0) {
    fragColor = vec4(0., 0., 0., 0.);
  } else if (picking_mode == 1) {
    fragColor = get_picking(window_id, scaled_window_coord);
  } else {
    if (   ((IG_DECORATION == atom_IG_DECORATION_SAVE) && (parent_IG_GHOST == atom_IG_GHOST))
               || ((IG_DECORATION == atom_IG_DECORATION_GHOSTS_ENABLE) && (parent_IG_GHOSTS_DISABLED != 1))) {
      fragColor = get_svg_content_any(scaled_window_coord, IG_CONTENT_ALT, IG_CONTENT_ALT_transform);
    } else {
      fragColor = get_svg_content(scaled_window_coord);
    }
    if (mouse_dist > 100.0) {
      fragColor.a *= 1. - (mouse_dist - 100.) / 50.;
    }
  }
}
