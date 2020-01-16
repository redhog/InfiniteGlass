#version 330 core

layout(lines) in;
layout(line_strip, max_vertices=4) out;

uniform ivec2 size;

uniform float IG_WORLD_ZOOM;

in vec2 coastline[];
out vec2 px_coord;

vec4 latlon2space(vec2 lonlat) {
  float phi = radians(lonlat[0]);
  float theta = radians(90. - lonlat[1]);
  return vec4(
    sin(theta) * sin(phi),
    cos(theta),
    sin(theta) * cos(phi),
    1.
  );
}

void main() {
  float frac = float(size.y) / float(size.x);
  
  mat4 sphere2gl = transpose(mat4(
    IG_WORLD_ZOOM * frac, 0., 0., 0.,
    0., IG_WORLD_ZOOM, 0., 0.,
    0., 0., IG_WORLD_ZOOM, min(0., -(IG_WORLD_ZOOM-1.)),
    0., 0., 0., 1.
  ));

  gl_Position = sphere2gl * latlon2space(coastline[0]);
  px_coord = vec2(size.x * (gl_Position.x + 1. / 2.), size.y * (1. - (gl_Position.y + 1. / 2.)));
  EmitVertex();
  gl_Position = sphere2gl * latlon2space(coastline[1]);
  px_coord = vec2(size.x * (gl_Position.x + 1. / 2.), size.y * (1. - (gl_Position.y + 1. / 2.)));
  EmitVertex();
  EndPrimitive();
}
