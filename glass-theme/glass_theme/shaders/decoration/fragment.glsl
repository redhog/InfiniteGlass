#version 330 core
precision highp float;

#include "resource://glass_theme/shaders/lib/fragment_svg.glsl"
#include "resource://glass_theme/shaders/lib/fragment_content.glsl"
#include "resource://glass_theme/shaders/lib/fragment_picking.glsl"

in vec2 px_window_bottom_left;
in vec2 px_window_top_right;
in vec2 px_coord;

uniform ivec2 size;
uniform int atom_IG_LAYER_DESKTOP;

uniform int atom_IG_DECORATION_GHOSTS_ENABLE;
uniform int atom_IG_DECORATION_GHOSTS_DISABLE;
uniform int atom_IG_DECORATION_SAVE;
uniform int atom_IG_DECORATION_RESTORE;
uniform int IG_DECORATION;

uniform int atom_IG_GHOST;
uniform int parent_IG_GHOST;
uniform int parent_IG_GHOSTS_DISABLED;

out vec4 fragColor;

void main() {
  if (   ((IG_DECORATION == atom_IG_DECORATION_SAVE) && (parent_IG_GHOST == atom_IG_GHOST))
      || ((IG_DECORATION == atom_IG_DECORATION_RESTORE) && (parent_IG_GHOST != atom_IG_GHOST))
      || ((IG_DECORATION == atom_IG_DECORATION_GHOSTS_ENABLE) && (parent_IG_GHOSTS_DISABLED == 1))
      || ((IG_DECORATION == atom_IG_DECORATION_GHOSTS_DISABLE) && (parent_IG_GHOSTS_DISABLED != 1))) {
    fragColor = vec4(0., 0., 0., 0.);
  } else {
    vec2 window_coord = px_coord - px_window_bottom_left;
    vec2 window_size = px_window_top_right - px_window_bottom_left;
    vec2 scaled_window_coord = vec2(window_coord.x / window_size.x, 1. - window_coord.y / window_size.y);

    vec2 texture_size = textureSize(window_sampler, 0);
    vec2 geometry_size = window_size / texture_size;
    float geometry_scale = sqrt(pow(geometry_size.x, 2) + pow(geometry_size.y, 2));

    if (picking_mode == 1) {
      fragColor = get_picking(window_id, scaled_window_coord);
    } else if (!isnan(IG_CONTENT_transform[0])) {
      fragColor = get_svg_content(scaled_window_coord);
    } else {
      fragColor = vec4(0., 0., 0., 0.);
    }
  }
}
