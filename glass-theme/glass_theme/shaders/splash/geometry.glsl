#version 150 core

layout(lines) in;
layout(line_strip, max_vertices=4) out;

uniform ivec2 size;

uniform float root_IG_WORLD_ZOOM;
uniform float root_IG_WORLD_LAT;
uniform float root_IG_WORLD_LON;

in vec2 coastline[];
out vec2 px_coord;

vec4 latlon2space(vec2 lonlat) {
  float lon = radians(-root_IG_WORLD_LON);
  float lat = radians(-root_IG_WORLD_LAT);
  mat4 rot = transpose(mat4(
    1., 0., 0., 0.,
    0., cos(lat), -sin(lat), 0.,
    0., sin(lat), cos(lat), 0.,
    0., 0., 0., 1.
  ));
  rot = rot * transpose(mat4(
    cos(lon), 0., -sin(lon), 0.,
    0., 1., 0., 0.,
    sin(lon), 0., cos(lon), 0.,
    0., 0., 0., 1.
  ));
  float phi = radians(180 - lonlat[0]);
  float theta = radians(90. - lonlat[1]);
  return rot * vec4(
    sin(theta) * sin(phi),
    cos(theta),
    sin(theta) * cos(phi),
    1.
  );
}

void main() {
  float frac = float(size.y) / float(size.x);
  
  mat4 sphere2gl = transpose(mat4(
    root_IG_WORLD_ZOOM * frac, 0., 0., 0.,
    0., root_IG_WORLD_ZOOM, 0., 0.,
    0., 0., root_IG_WORLD_ZOOM, max(1., root_IG_WORLD_ZOOM),
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
