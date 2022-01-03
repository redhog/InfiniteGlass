# Styling

All rendering in InfiniteGlass is controlled by the OpenGL shader code
and window properties. These are set up by the current theme. The
theme is selected and configured in
[~/.config/glass/theme.yml](../glass-config-init/glass_config_init/theme.yml),
and is implemented as a single python class that subclasses
[glass_theme.base.ThemeBase](../glass-theme/glass_theme/base.py). The
arguments in theme.json can override any member variable defined in
the theme class. For an example theme, see
[glass_theme.default.Theme](../glass-theme/glass_theme/default.py).

## Shaders

The shaders to be loaded are specified using three parameters:

    shader_path = "resource://glass_theme/shaders"
    shaders = ("DEFAULT", "ROOT", "SPLASH", "SPLASH_BACKGROUND")
    shader_parts = ("GEOMETRY", "VERTEX", "FRAGMENT")

This will load the listed shaders from the specified path (python
pkg_resource or plain file path), by appending the lower case shader
and part names to the shader path, e.g.
"resource://glass_theme/shaders/splash_background/geometry.glsl".
These glsl files can include other files using the C include syntax,
e.g.

    #include "resource://glass_theme/shaders/lib/picking.glsl"

In particular, all coordinate handling, SVG rendering and picking
(mouse handling) code is available as functions in different glsl
files in the "glass_theme/shaders/lib".

# Widgets
Desktop widgets and window decorations, that is, clickable action buttons, are defined in [~/.config/glass/widgets.yml](../glass-config-init/glass_config_init/widgets.yml). They each define a window, typically setting an SVG image as content, and a click action.
The click actions are the same ones available when defining [keyboard shortcuts](KEYMAP.md).
