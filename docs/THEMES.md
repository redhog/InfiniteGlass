All rendering in InfiniteGlass is controlled by the OpenGL shader code
and window properties. These are set up by the current theme. The
theme is selected and configured in
[~/.config/glass/theme.json](../glass-config-init/glass_config_init/theme.json),
and is implemented as a single python class that subclasses
glass_theme.base.ThemeBase. The arguments in theme.json can override
any member variable defined in the theme class.

# Shaders

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
