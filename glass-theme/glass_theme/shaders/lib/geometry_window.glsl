#include "resource://glass_theme/shaders/lib/geometry_coords.glsl"

uniform sampler2D window_sampler;
uniform vec4 IG_COORDS;
out vec2 px_coord;
flat out ivec2 px_window_bottom_left;
flat out ivec2 px_window_top_right;

void set_px_window_pos(void) {
  ivec2 px_bottom_left;
  ivec2 px_top_right;

  float left = IG_COORDS[0];
  float top = IG_COORDS[1];
  float right = left + IG_COORDS[2];
  float bottom = top - IG_COORDS[3];

  ivec2 texture_size = ivec2(textureSize(window_sampler, 0));

  px_bottom_left = glscreen2pixel(space2glscreen(vec2(left, bottom)));
  px_top_right = glscreen2pixel(space2glscreen(vec2(right, top)));

  if (abs((px_top_right - px_bottom_left).x - texture_size.x) < 10) {
    px_top_right.x = px_bottom_left.x + texture_size.x;
  }
  if (abs((px_top_right - px_bottom_left).y - texture_size.y) < 10) {
    px_top_right.y = px_bottom_left.y + texture_size.y;
  }

  px_window_bottom_left = px_bottom_left;
  px_window_top_right = px_top_right;
}


void emit_window(ivec2 px_bottom_left, ivec2 px_top_right) {
  ivec2 px_top_left = ivec2(px_bottom_left.x, px_top_right.y);
  ivec2 px_bottom_right = ivec2(px_top_right.x, px_bottom_left.y);

  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_left));
  px_coord = vec2(pixelclipscreen(px_bottom_left).xy);
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_left));
  px_coord = vec2(pixelclipscreen(px_top_left).xy);
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_right));
  px_coord = vec2(pixelclipscreen(px_bottom_right).xy);
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_right));
  px_coord = vec2(pixelclipscreen(px_top_right).xy);
  EmitVertex();

  EndPrimitive();
}
