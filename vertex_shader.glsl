#version 330 // Specify which version of GLSL we are using.

in vec4 coords; // x,y,w,h in space 
out vec4 window; // x,y,w,h in space

void main() {
  window = coords;
  //gl_Position = vec4();
}
