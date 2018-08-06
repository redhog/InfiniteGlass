#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;
 
uniform vec4 screen; // x,y,w,h in space
in vec4 window[]; // x,y,w,h in space

out vec2 UV;

mat4 screen2glscreen = transpose(mat4(
  2., 0., 0., -1.,
  0., 2., 0., -1.,
  0., 0., 1., 0.,
  0., 0., 0., 1.
));

void main() {
  float left, right, top, bottom;

  mat4 space2screen = screen2glscreen * transpose(mat4(
    1./screen[2], 0., 0., -screen.x/screen[2],
    0., 1./screen[3], 0., -screen.y/screen[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));

  for(int i = 0; i < gl_in.length(); i++) {

    left = window[i][0];
    top = window[i][1];
    right = left + window[i][2];
    bottom = top - window[i][3];
    
    gl_Position = space2screen * vec4(left, bottom, 0., 1.);
    UV = vec2(0., 1.);
    EmitVertex();

    gl_Position = space2screen * vec4(left, top, 0., 1.);
    UV = vec2(0., 0.);
    EmitVertex();

    gl_Position = space2screen * vec4(right, bottom, 0., 1.);
    UV = vec2(1., 1.);
    EmitVertex();

    gl_Position = space2screen * vec4(right, top, 0., 1.);
    UV = vec2(1., 0.);
    EmitVertex();
  }
  EndPrimitive();
}
