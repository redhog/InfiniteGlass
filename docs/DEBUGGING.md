# Logging

The InfiniteGlass components have extensive logging / debug print
facilities. These are turned on and of using a fine grained
environment variable system. These variables can be set in your shell
when running 'make run', or in
[~/.config/glass/session.sh](../glass-config-init/glass_config_init/session.sh).

All such environment variable names begin with 'GLASS_DEBUG'.

Each logging statement has a path, which is printed before the
message. Example:

    GLASS_DEBUG.renderer.item_window_svg.c.item_type_window_svg_update_drawing.window.svg: RENDER -85,-84[512,512] = [85,85]

The path can be used to turn on or off the printing of the message in
question. The path is turned into the name of an environment variable
by replacing any dot (.) with an underscore (_). The value of the
variable must be either 1 (do print), or 0 (do not print). Example:

    GLASS_DEBUG_renderer_item_window_svg_c_item_type_window_svg_update_drawing_window_svg=1

Groups of messages can be turned on or off all at the same time using a path prefix, e.g.

    GLASS_DEBUG_renderer_item_window_svg_c_item_type_window=1

If multiple such prefixes match a particular message (some of them
possibly set to 0, others to 1), the longest one is used. For example, if both

    GLASS_DEBUG_renderer_item_window_svg_c_item_type_window_svg_update_drawing_window_svg=0
    GLASS_DEBUG_renderer_item_window_svg_c_item_type_window=1

are defined, the above logging line would not be printed, as the
longer matching variable is det to 0.

# Client messages

The renderer accepts a set of `ClientMessage`:s that causes it to print lists of objects in its memory to its standard
out - items (windows), views and shaders. These messages have to be sent to the root window with a SubstructureNotifyMask.
This can be achived easily using the `glass-action` command line tool:

```
glass-action window send --mask SubstructureNotifyMask --window root IG_DEBUG_LIST_ITEMS 
glass-action window send --mask SubstructureNotifyMask --window root IG_DEBUG_LIST_SHADERS
glass-action window send --mask SubstructureNotifyMask --window root IG_DEBUG_LIST_VIEWS
```
