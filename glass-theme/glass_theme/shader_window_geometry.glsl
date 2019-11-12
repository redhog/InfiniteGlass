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

vec2 snap2pixel(vec2 xy) {
  return vec2(floor(xy.x * size[0] / 2.) * 2. / size[0],
              floor(xy.y * size[1] / 2.) * 2. / size[1]);
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

  float margin_left = 6. * 2. / width;
  float margin_right = 6. * 2. / width;
  float margin_top = 6. * 2. / height;
  float margin_bottom = 6. * 2. / height;

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

  gl_Position = space2screen * vec4(left, bottom, 0., 1.) + vec4(-margin_left, -margin_bottom, 0., 0.);
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = space2screen * vec4(left, top, 0., 1.) + vec4(-margin_left, +margin_top, 0., 0.);
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = space2screen * vec4(right, bottom, 0., 1.) + vec4(margin_right, -margin_bottom, 0., 0.);
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  gl_Position = space2screen * vec4(right, top, 0., 1.) + vec4(margin_right, margin_top, 0., 0.);
  window_coord = (space2windowcoord * screen2space * gl_Position).xy;
  geometry_size = window_size;
  EmitVertex();

  EndPrimitive();
}
