#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform vec4 IG_COORDS;
uniform ivec2 IG_SIZE;
uniform vec4 screen; // x,y,w,h in space
uniform ivec2 size;

out vec2 window_coord_per_pixel;
out vec2 window_coord;
out float geometry_size;

ivec4 pixelclipscreen(ivec4 xy) {
  if (xy.x < 0) xy.x = 0;
  if (xy.x >= size.x) xy.x = size.x -1;
  if (xy.y < 0) xy.y = 0;
  if (xy.y >= size.y) xy.y = size.y -1;
  return xy;
}

vec4 pixel2glscreen(ivec4 xy) {
  return transpose(mat4(
    2./size[0], 0., 0., -1.,
    0., 2./size[1], 0., -1.,
    0., 0., 0., 0.,
    0., 0., 0., 1.
  )) * vec4(xy);
}

ivec4 glscreen2pixel(vec4 xy) {
  return ivec4(transpose(mat4(
    size[0]/2., 0., 0., size[0]/2.,
    0., size[1]/2., 0., size[1]/2.,
    0., 0., 0., 0.,
    0., 0., 0., 1.
  )) * xy);
}

mat4 screen2glscreen = transpose(mat4(
  2., 0., 0., -1.,
  0., 2., 0., -1.,
  0., 0., 1., 0.,
  0., 0., 0., 1.
));

void main() {
  int width = size[0];
  int height = size[1];

  int margin_left = 6;
  int margin_right = 6;
  int margin_top = 6;
  int margin_bottom = 6;

  mat4 space2screen = screen2glscreen * transpose(mat4(
    1./screen[2], 0., 0., -screen.x/screen[2],
    0., 1./screen[3], 0., -screen.y/screen[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));

  mat4 screen2space = inverse(space2screen);

  float left = IG_COORDS[0];
  float top = IG_COORDS[1];
  float right = left + IG_COORDS[2];
  float bottom = top - IG_COORDS[3];

  mat4 space2windowcoord = transpose(mat4(
     1/IG_COORDS[2], 0.,  0., -IG_COORDS[0]/IG_COORDS[2],
     0., -1./IG_COORDS[3], 0., IG_COORDS[1]/IG_COORDS[3],
     0., 0., 1., 0.,
     0., 0., 0., 1.
  ));

  float window_size = distance(space2screen * vec4(left, bottom, 0., 1.),
                               space2screen * vec4(right, top, 0., 1.));

  window_coord_per_pixel = abs(  (space2windowcoord * screen2space * vec4(2./width, 2./height, 0., 0.)).xy
                               - (space2windowcoord * screen2space * vec4(0., 0., 0., 0.)).xy);

  ivec4 px_bottom_left = glscreen2pixel(space2screen * vec4(left, bottom, 0., 1.));
  ivec4 px_top_right = glscreen2pixel(space2screen * vec4(right, top, 0., 1.));
  
  if (abs((px_top_right - px_bottom_left).x - width) < 2) {
    px_top_right.x = px_bottom_left.x + width;
  }
  if (abs((px_top_right - px_bottom_left).y - height) < 2) {
    px_top_right.y = px_bottom_left.y + height;
  }
  px_bottom_left = px_bottom_left - ivec4(margin_left, margin_bottom, 0, 0);
  px_top_right = px_top_right + ivec4(margin_right, margin_top, 0, 0);

  ivec4 px_top_left = ivec4(px_bottom_left.x, px_top_right.y, 0, 1);
  ivec4 px_bottom_right = ivec4(px_top_right.x, px_bottom_left.y, 0, 1);

  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_left));
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_left));
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_bottom_right));
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = pixel2glscreen(pixelclipscreen(px_top_right));
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  EndPrimitive();
}
