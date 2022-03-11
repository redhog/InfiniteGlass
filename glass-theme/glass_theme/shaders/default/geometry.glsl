#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include "resource://glass_theme/shaders/lib/geometry_window.glsl"


out float is_edge_hint;

void main() {
  set_px_window_pos();

  ivec2 px_bottom_left = px_window_bottom_left - ivec2(margin_left, margin_bottom);
  ivec2 px_top_right = px_window_top_right + ivec2(margin_right, margin_top);

  // Edge hinting for windows outside the desktop...
  is_edge_hint = 0;
  if (px_bottom_left.x >= size.x) {
    px_bottom_left.x = size.x - EDGE_HINT_WIDTH;
    
    px_top_right.x = size.x;
    is_edge_hint = 1;
  }
  if (px_top_right.x < 0) {
    px_bottom_left.x = 0;
    px_top_right.x = EDGE_HINT_WIDTH;
    is_edge_hint = 1;
  }
  if (px_bottom_left.y >= size.y) {
    px_bottom_left.y = size.y - EDGE_HINT_WIDTH;
    px_top_right.y = size.y;
    is_edge_hint = 1;
  }
  if (px_top_right.y < 0) {
    px_bottom_left.y = 0;
    px_top_right.y = EDGE_HINT_WIDTH;
    is_edge_hint = 1;
  }

  emit_window(px_bottom_left, px_top_right);
}
