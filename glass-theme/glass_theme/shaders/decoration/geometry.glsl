#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include "resource://glass_theme/shaders/lib/geometry_coords.glsl"

uniform sampler2D window_sampler;
uniform vec4 IG_COORDS;

uniform vec4 parent_IG_COORDS;

out vec2 px_window_bottom_left;
out vec2 px_window_top_right;
out vec2 px_coord;

out vec2 px_parent_window_bottom_left;
out vec2 px_parent_window_top_right;


void main() {
  vec2 px_bottom_left;
  vec2 px_top_right;
  vec2 px_top_left;
  vec2 px_bottom_right;

  float left = IG_COORDS[0] + 0.00000000001 * parent_IG_COORDS[0];
  float top = IG_COORDS[1];
  float right = left + IG_COORDS[2];
  float bottom = top - IG_COORDS[3];

  px_bottom_left = glscreen2pixel(space2glscreen(vec2(left, bottom)));
  px_top_right = glscreen2pixel(space2glscreen(vec2(right, top)));

  vec2 texture_size = textureSize(window_sampler, 0);


  if (abs((px_top_right - px_bottom_left).x - texture_size.x) < 10) {
    px_top_right.x = px_bottom_left.x + texture_size.x;
  }
  if (abs((px_top_right - px_bottom_left).y - texture_size.y) < 10) {
    px_top_right.y = px_bottom_left.y + texture_size.y;
  }

  px_window_bottom_left = px_bottom_left;
  px_window_top_right = px_top_right;

  px_top_left = ivec2(px_bottom_left.x, px_top_right.y);
  px_bottom_right = ivec2(px_top_right.x, px_bottom_left.y);


  float parent_left = parent_IG_COORDS[0];
  float parent_top = parent_IG_COORDS[1];
  float parent_right = parent_left + parent_IG_COORDS[2];
  float parent_bottom = parent_top - parent_IG_COORDS[3];

  px_parent_window_bottom_left = glscreen2pixel(space2glscreen(vec2(parent_left, parent_bottom)));
  px_parent_window_top_right = glscreen2pixel(space2glscreen(vec2(parent_right, parent_top)));


  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_left));
  px_coord = pixelclipscreen(px_bottom_left).xy;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_left));
  px_coord = pixelclipscreen(px_top_left).xy;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_right));
  px_coord = pixelclipscreen(px_bottom_right).xy;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_right));
  px_coord = pixelclipscreen(px_top_right).xy;
  EmitVertex();

  EndPrimitive();
}
