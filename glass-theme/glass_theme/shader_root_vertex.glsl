#version 330 core

in vec2 SERPENT;
out vec2 serpent;

void main() {
  serpent = SERPENT;
  gl_Position = vec4(0., 0., 0., 1.);
}
