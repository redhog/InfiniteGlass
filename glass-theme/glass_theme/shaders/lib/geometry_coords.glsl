uniform vec4 screen; // x,y,w,h in space
uniform ivec2 size;

ivec2 pixelclipscreen(ivec2 xy) {
  if (xy.x < 0) xy.x = 0;
  if (xy.x >= size.x) xy.x = size.x -1;
  if (xy.y < 0) xy.y = 0;
  if (xy.y >= size.y) xy.y = size.y -1;
  return xy;
}

vec4 pixel2glscreen(ivec2 xy) {
  return transpose(mat4(
    2./size.x, 0., 0., -1.,
    0., 2./size.y, 0., -1.,
    0., 0., 0., 1.,
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

vec4 space2glscreen(vec2 xy) {
  return screen2glscreen * transpose(mat4(
    1./screen[2], 0., 0., -screen.x/screen[2],
    0., 1./screen[3], 0., -screen.y/screen[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  )) * vec4(xy, 0., 1.);
}
