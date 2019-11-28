#version 330 core
precision highp float;

#define ICON_CUTOFF_1 .5
#define ICON_CUTOFF_2 .3

in vec2 window_coord_per_pixel;
in vec2 window_coord;
in float geometry_size;

uniform ivec2 size;
uniform int picking_mode;
uniform float window_id;
uniform int window;

uniform int atom_IG_LAYER_MENU;
uniform int atom__NET_WM_WINDOW_TYPE_NORMAL;

uniform sampler2D window_sampler;
uniform sampler2D WM_HINTS_icon;
uniform sampler2D WM_HINTS_icon_mask;
uniform int WM_HINTS_icon_enabled;
uniform int WM_HINTS_icon_mask_enabled;
uniform sampler2D _NET_WM_ICON;
uniform int _NET_WM_ICON_enabled;
uniform sampler2D IG_CONTENT;
uniform vec4 IG_CONTENT_transform;
uniform int IG_LAYER;
uniform int _NET_WM_WINDOW_TYPE;

uniform int root__NET_ACTIVE_WINDOW;


out vec4 fragColor;

void draw_border() {
  if (IG_LAYER == atom_IG_LAYER_MENU || (_NET_WM_WINDOW_TYPE != 0 &&_NET_WM_WINDOW_TYPE != atom__NET_WM_WINDOW_TYPE_NORMAL)) {
    fragColor = vec4(0., 0., 0., 0.);
  } else if (root__NET_ACTIVE_WINDOW == window) {
    vec2 dcoord = window_coord;
    if (dcoord.x >= 0. && dcoord.x <= 1.) dcoord.x = 0.;
    if (dcoord.y >= 0. && dcoord.y <= 1.) dcoord.y = 0.;
    if (dcoord.x > 1.) dcoord.x = dcoord.x - 1.;
    if (dcoord.y > 1.) dcoord.y = dcoord.y - 1.;
    ivec2 dist = ivec2(ceil(abs(dcoord) / window_coord_per_pixel));
    if ((dist.x == 3 && abs(dist.y) < 4) || (dist.y == 3 && abs(dist.x) < 4)) {
      fragColor = vec4(0., 0., 0., 1.);
    } else if ((dist.x == 4 && abs(dist.y) < 5) || (dist.y == 4 && abs(dist.x) < 5)) {
      fragColor = vec4(1., 1., 1., 1.);
    } else if (dist.x > 4 || dist.y > 4) {
      fragColor = vec4(0., 0., 0., 1.);
    } else {
      fragColor = vec4(0., 0., 0., 0.);
    }
  } else {
    vec2 dcoord = window_coord;
    if (dcoord.x >= 0. && dcoord.x <= 1.) dcoord.x = 0.;
    if (dcoord.y >= 0. && dcoord.y <= 1.) dcoord.y = 0.;
    if (dcoord.x > 1.) dcoord.x = dcoord.x - 1.;
    if (dcoord.y > 1.) dcoord.y = dcoord.y - 1.;
    ivec2 dist = ivec2(ceil(abs(dcoord) / window_coord_per_pixel));
    if (dist.x > 5 || dist.y > 5) {
      fragColor = vec4(0., 0., 0., 1.);
    } else {
      fragColor = vec4(0., 0., 0., 0.);
    }
  }
}

void draw_svg_content() {
  mat4 transform_mat = transpose(mat4(
    1./IG_CONTENT_transform[2], 0., 0., -IG_CONTENT_transform[0]/IG_CONTENT_transform[2],
    0., 1./IG_CONTENT_transform[3], 0., -IG_CONTENT_transform[1]/IG_CONTENT_transform[3],
    0., 0., 1., 0.,
    0., 0., 0., 1.
  ));
  vec4 texture_coord = transform_mat * vec4(window_coord, 0, 1.);
  fragColor = texture(IG_CONTENT, texture_coord.xy).rgba;
}

vec4 get_icon() {
  vec4 icon_color;
  if (_NET_WM_ICON_enabled ==  1) {
    icon_color = texture(_NET_WM_ICON, window_coord).rgba;
  } else {
    if (WM_HINTS_icon_enabled == 1) {
      icon_color = texture(WM_HINTS_icon, window_coord).rgba;
    } else {
      icon_color =  vec4(1., 0., 0., 1.);
    }
    if (WM_HINTS_icon_mask_enabled == 1) {
      icon_color.a = 1. - texture(WM_HINTS_icon_mask, window_coord).r;
    }
  }
  return icon_color;
}

vec4 get_pixmap() {
  return texture(window_sampler, window_coord).rgba;
}

void main() {
  if (picking_mode == 1) {
    if (window_coord.x < 0. || window_coord.x > 1. || window_coord.y < 0. || window_coord.y > 1.) {
      fragColor = vec4(0.,0.,0.,0.);
    } else {
      fragColor = vec4(window_coord.x, window_coord.y, window_id, 1.);
    }
  } else if (window_coord.x < 0. || window_coord.x > 1. || window_coord.y < 0. || window_coord.y > 1.) {
    draw_border();
  } else if (!isnan(IG_CONTENT_transform[0])) {
    draw_svg_content();
  } else if (IG_LAYER == atom_IG_LAYER_MENU || geometry_size > ICON_CUTOFF_1) {
    fragColor = get_pixmap();
  } else if (geometry_size > ICON_CUTOFF_2) {
    float scale = (geometry_size - ICON_CUTOFF_2) / (ICON_CUTOFF_1 - ICON_CUTOFF_2);
    fragColor = scale * get_pixmap() + (1 - scale) * get_icon();
  } else {
    fragColor = get_icon();
  }
}
