#version 330 core
precision highp float;

#define ICON_CUTOFF_1 .5
#define ICON_CUTOFF_2 .3

in vec2 window_coord;
in float geometry_size;

uniform sampler2D window_sampler;
uniform sampler2D WM_HINTS_icon;
uniform sampler2D WM_HINTS_icon_mask;
uniform int WM_HINTS_icon_enabled;
uniform int WM_HINTS_icon_mask_enabled;
uniform int picking_mode;
uniform float window_id;
uniform sampler2D IG_CONTENT;
uniform vec4 IG_CONTENT_transform;

out vec4 fragColor;


vec4 icon_color;
vec4 window_color;

float scale;

void main() {
  fragColor = vec4(window_coord.x, window_coord.y, window_id, 1.);
}
