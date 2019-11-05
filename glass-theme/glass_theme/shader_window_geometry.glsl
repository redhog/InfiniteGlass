#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform vec4 IG_COORDS;
uniform ivec2 IG_SIZE;
uniform vec4 screen; // x,y,w,h in space

out vec2 window_coord;
out float geometry_size;

mat4 screen2glscreen = transpose(mat4(
  2., 0., 0., -1.,
  0., 2., 0., -1.,
  0., 0., 1., 0.,
  0., 0., 0., 1.
));

void main() {
  float left, right, top, bottom, size;
  int width = IG_SIZE[0];
  int height = IG_SIZE[1];

  mat4 space2screen = screen2glscreen * transpose(mat4(
    1./screen[2], 0., 0., -screen.x/screen[2],
    0., 1./screen[3], 0., -screen.y/screen[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));

  left = IG_COORDS[0];
  top = IG_COORDS[1];
  right = left + IG_COORDS[2];
  bottom = top - IG_COORDS[3];

  left += 0.000000001 * IG_SIZE[0];

  left = floor(left * width / 2.) * 2. / width;
  right = floor(right * width / 2.) * 2. / width;
  bottom = floor(bottom * width / 2.) * 2. / width;
  top = floor(top * width / 2.) * 2. / width;

  size = distance(space2screen * vec4(left, bottom, 0., 1.),
                  space2screen * vec4(right, top, 0., 1.));

  gl_Position = space2screen * vec4(left, bottom, 0., 1.);
  window_coord = vec2(0., 1.);
  geometry_size = size;
  EmitVertex();

  gl_Position = space2screen * vec4(left, top, 0., 1.);
  window_coord = vec2(0., 0.);
  geometry_size = size;
  EmitVertex();

  gl_Position = space2screen * vec4(right, bottom, 0., 1.);
  window_coord = vec2(1., 1.);
  geometry_size = size;
  EmitVertex();

  gl_Position = space2screen * vec4(right, top, 0., 1.);
  window_coord = vec2(1., 0.);
  geometry_size = size;
  EmitVertex();

  EndPrimitive();
}
