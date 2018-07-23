#version 130 // Specify which version of GLSL we are using.

// in_Position was bound to attribute index 0("shaderAttribute")
in  vec2 space_pos;
in  vec2 win_pos;
out vec2 UV;

void main() {
    UV = vec2(win_pos.x, win_pos.y);
    gl_Position = vec4(space_pos.x, space_pos.y, 0.0, 1.0);
}