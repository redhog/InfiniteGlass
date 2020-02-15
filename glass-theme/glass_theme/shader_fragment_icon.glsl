uniform sampler2D WM_HINTS_icon;
uniform sampler2D WM_HINTS_icon_mask;
uniform int WM_HINTS_icon_enabled;
uniform int WM_HINTS_icon_mask_enabled;
uniform sampler2D _NET_WM_ICON;
uniform int _NET_WM_ICON_enabled;

vec4 get_icon(vec2 scaled_window_coord) {
  vec4 icon_color;
  if (_NET_WM_ICON_enabled ==  1) {
    icon_color = texture(_NET_WM_ICON, scaled_window_coord).rgba;
  } else {
    if (WM_HINTS_icon_enabled == 1) {
      icon_color = texture(WM_HINTS_icon, scaled_window_coord).rgba;
    } else {
      icon_color =  vec4(1., 0., 0., 1.);
    }
    if (WM_HINTS_icon_mask_enabled == 1) {
      icon_color.a = 1. - texture(WM_HINTS_icon_mask, scaled_window_coord).r;
    }
  }
  return icon_color;
}
