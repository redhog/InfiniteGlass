#version 150 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include "resource://glass_theme/shaders/lib/geometry_window.glsl"

uniform vec4 parent_IG_COORDS;
flat out ivec2 px_parent_window_bottom_left;
flat out ivec2 px_parent_window_top_right;

void main() {
  set_px_window_pos();

  float parent_left = parent_IG_COORDS[0];
  float parent_top = parent_IG_COORDS[1];
  float parent_right = parent_left + parent_IG_COORDS[2];
  float parent_bottom = parent_top - parent_IG_COORDS[3];

  px_parent_window_bottom_left = glscreen2pixel(space2glscreen(vec2(parent_left, parent_bottom)));
  px_parent_window_top_right = glscreen2pixel(space2glscreen(vec2(parent_right, parent_top)));

  emit_window(px_window_bottom_left, px_window_top_right);
}
