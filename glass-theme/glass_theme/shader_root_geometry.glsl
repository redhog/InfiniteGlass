#version 330 core

layout(lines) in;
layout(line_strip, max_vertices=4) out;

uniform ivec2 size;

in vec2 serpent[];
out vec2 px_coord;

void main() {
  gl_Position = vec4(serpent[0].x, serpent[0].y, 0., 1.);
  px_coord = vec2(size.x * (serpent[0].x + 1. / 2.), size.y * (1. - (serpent[0].y + 1. / 2.)));
  EmitVertex();
  gl_Position = vec4(serpent[1].x, serpent[1].y, 0., 1.);
  px_coord = vec2(size.x * (serpent[1].x + 1. / 2.), size.y * (1. - (serpent[1].y + 1. / 2.)));
  EmitVertex();
  EndPrimitive();
}
