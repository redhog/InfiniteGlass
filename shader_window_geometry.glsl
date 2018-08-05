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
  vec2 p0, p1, p2, p3;

  mat4 space2screen = screen2glscreen * transpose(mat4(
    1./screen[2], 0., 0., -screen.x/screen[2],
    0., 1./screen[3], 0., -screen.y/screen[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));

  for(int i = 0; i < gl_in.length(); i++) {
    p1 = vec2(window[i][0], window[i][1]); // l, t
    p2 = p1 + vec2(window[i][2], window[i][3]); // r, b
    p0 = vec2(p1.x, p2.y); // l, b
    p3 = vec2(p2.x, p1.y); // r, t

    gl_Position = space2screen * vec4(p0.x, p0.y, 0., 1.);
    UV = vec2(0., 1.);
    EmitVertex();

    gl_Position = space2screen * vec4(p1.x, p1.y, 0., 1.);
    UV = vec2(0., 0.);
    EmitVertex();

    gl_Position = space2screen * vec4(p2.x, p2.y, 0., 1.);
    UV = vec2(1., 1.);
    EmitVertex();

    gl_Position = space2screen * vec4(p3.x, p3.y, 0., 1.);
    UV = vec2(1., 0.);
    EmitVertex();
  }
/*
    gl_Position = vec4(-.5, -.5, 0., 1.);
    UV = vec2(0., 0.);
    EmitVertex();
    gl_Position = vec4(-.5, .5, 0., 1.);
    UV = vec2(0., 1.);
    EmitVertex();
    gl_Position = vec4(.5, .5, 0., 1.);
    UV = vec2(1., 1.);
    EmitVertex();
    gl_Position = vec4(.5, -.5, 0., 1.);
    UV = vec2(1., 0.);
    EmitVertex();
*/
  EndPrimitive();
}
