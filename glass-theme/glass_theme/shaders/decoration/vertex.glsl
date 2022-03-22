#version 150 core

in vec4 coords; // x,y,w,h in space 
out vec4 window; // x,y,w,h in space

void main() {
  window = coords;
  gl_Position = vec4(0., 0., 0., 1.);
}
