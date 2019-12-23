#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;


uniform sampler2D window_sampler;
uniform vec4 IG_COORDS;
uniform vec4 screen; // x,y,w,h in space
uniform ivec2 size;

out vec2 px_window_bottom_left;
out vec2 px_window_top_right;
out vec2 px_coord;
out float geometry_size;


vec2 pixelclipscreen(vec2 xy) {
  if (xy.x < 0) xy.x = 0;
  if (xy.x >= size.x) xy.x = size.x -1;
  if (xy.y < 0) xy.y = 0;
  if (xy.y >= size.y) xy.y = size.y -1;
  return xy;
}

vec4 pixel2glscreen(vec2 xy) {
  return transpose(mat4(
    2./size.x, 0., 0., -1.,
    0., 2./size.y, 0., -1.,
    0., 0., 0., 0.,
    0., 0., 0., 1.
  )) * vec4(xy, 0., 1.);
}

ivec2 glscreen2pixel(vec4 xy) {
  return ivec4(transpose(mat4(
    size.x/2., 0., 0., size.x/2.,
    0., size.y/2., 0., size.y/2.,
    0., 0., 0., 0.,
    0., 0., 0., 1.
  )) * xy).xy;
}

mat4 screen2glscreen = transpose(mat4(
  2., 0., 0., -1.,
  0., 2., 0., -1.,
  0., 0., 1., 0.,
  0., 0., 0., 1.
));

void main() {
  int margin_left = 6;
  int margin_right = 6;
  int margin_top = 6;
  int margin_bottom = 6;

  vec2 px_bottom_left;
  vec2 px_top_right;
  vec2 px_top_left;
  vec2 px_bottom_right;

  mat4 space2screen = screen2glscreen * transpose(mat4(
    1./screen[2], 0., 0., -screen.x/screen[2],
    0., 1./screen[3], 0., -screen.y/screen[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));

  float left = IG_COORDS[0];
  float top = IG_COORDS[1];
  float right = left + IG_COORDS[2];
  float bottom = top - IG_COORDS[3];

  float window_size = distance(space2screen * vec4(left, bottom, 0., 1.),
                               space2screen * vec4(right, top, 0., 1.));

  px_bottom_left = glscreen2pixel(space2screen * vec4(left, bottom, 0., 1.));
  px_top_right = glscreen2pixel(space2screen * vec4(right, top, 0., 1.));

  vec2 texture_size = textureSize(window_sampler, 0);


  if (abs((px_top_right - px_bottom_left).x - texture_size.x) < 10) {
    px_top_right.x = px_bottom_left.x + texture_size.x;
  }
  if (abs((px_top_right - px_bottom_left).y - texture_size.y) < 10) {
    px_top_right.y = px_bottom_left.y + texture_size.y;
  }

  px_window_bottom_left = px_bottom_left;
  px_window_top_right = px_top_right;

  px_bottom_left = px_bottom_left - vec2(margin_left, margin_bottom);
  px_top_right = px_top_right + vec2(margin_right, margin_top);

  px_top_left = ivec2(px_bottom_left.x, px_top_right.y);
  px_bottom_right = ivec2(px_top_right.x, px_bottom_left.y);

  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_left));
  px_coord = pixelclipscreen(px_bottom_left).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_left));
  px_coord = pixelclipscreen(px_top_left).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_right));
  px_coord = pixelclipscreen(px_bottom_right).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_right));
  px_coord = pixelclipscreen(px_top_right).xy;
  geometry_size = window_size;
  EmitVertex();

  EndPrimitive();
}
