#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform ivec2 size;

out vec2 px_coord;

void main() {

  gl_Position = vec4(-1., -1., 0., 1.);
  px_coord = vec2(0., size.y);
  EmitVertex();  
  gl_Position = vec4(-1., 1., 0., 1.);
  px_coord = vec2(0., 0.);
  EmitVertex();
  gl_Position = vec4(1., -1., 0., 1.);
  px_coord = vec2(size.x, size.y);
  EmitVertex();
  gl_Position = vec4(1., 1., 0., 1.);
  px_coord = vec2(size.x, 0.);
  EmitVertex();

  EndPrimitive();
}
