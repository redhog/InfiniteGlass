uniform int IG_LAYER;

uniform int atom_IG_LAYER_MENU;
uniform int atom__NET_WM_WINDOW_TYPE_NORMAL;

uniform int _NET_WM_WINDOW_TYPE;
uniform int root__NET_ACTIVE_WINDOW;

uniform int IG_WINDOW_TYPE;
uniform int atom_IG_ISLAND;

vec4 get_border(int window_id, vec2 window_coord, vec2 window_size) {
  vec2 dcoord = window_coord;
  if (dcoord.x >= 0. && dcoord.x < window_size.x) dcoord.x = 0.;
  if (dcoord.y >= 0. && dcoord.y < window_size.y) dcoord.y = 0.;
  if (dcoord.x >= window_size.x) dcoord.x = dcoord.x - window_size.x;
  if (dcoord.y >= window_size.y) dcoord.y = dcoord.y - window_size.y;
  ivec2 dist = ivec2(ceil(abs(dcoord)));

  int radius = dist.x > dist.y ? dist.x : dist.y;

  if (IG_LAYER == atom_IG_LAYER_MENU || (_NET_WM_WINDOW_TYPE != 0 && _NET_WM_WINDOW_TYPE != atom__NET_WM_WINDOW_TYPE_NORMAL && IG_WINDOW_TYPE != atom_IG_ISLAND)) {
    return vec4(0., 0., 0., 0.);
  } else if (root__NET_ACTIVE_WINDOW == window_id) {
    if (radius == 1) {
      return BORDER_ACTIVE_COLOR_1;
    } else if (radius == 2) {
      return BORDER_ACTIVE_COLOR_2;
    } else if (radius == 3) {
      return BORDER_ACTIVE_COLOR_3;
    } else if (radius == 4) {
      return BORDER_ACTIVE_COLOR_4;
    } else if (radius == 5) {
      return BORDER_ACTIVE_COLOR_5;
    } else if (radius == 6) {
      return BORDER_ACTIVE_COLOR_6;
    }
  } else {
    if (radius == 1) {
      return BORDER_COLOR_1;
    } else if (radius == 2) {
      return BORDER_COLOR_2;
    } else if (radius == 3) {
      return BORDER_COLOR_3;
    } else if (radius == 4) {
      return BORDER_COLOR_4;
    } else if (radius == 5) {
      return BORDER_COLOR_5;
    } else if (radius == 6) {
      return BORDER_COLOR_6;
    }
  }
}
