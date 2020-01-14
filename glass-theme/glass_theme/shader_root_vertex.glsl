#version 330 core

in vec2 IG_COASTLINE;
out vec2 coastline;

void main() {
  coastline = IG_COASTLINE;
  gl_Position = vec4(0., 0., 0., 1.);
}
