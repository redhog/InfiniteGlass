import InfiniteGlass
import pkg_resources
import json
import numpy

def linestrings2texture(f):
    coastline = json.load(f)

    size = 0
    for feature in coastline["features"]:
        size += len(feature["geometry"]["coordinates"])

    res = []
    for feature in coastline["features"]:
        coords = feature["geometry"]["coordinates"]
        coords = list(zip(coords[:-1], coords[1:]))
        res.extend([z for x in coords for y in x for z in y])

    return res
        
def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        display.root["IG_VIEW_MENU_LAYER"] = "IG_LAYER_MENU"
        display.root["IG_VIEW_MENU_VIEW"] = [0.0, 0.0, 1.0, 0.0]
        display.root["IG_VIEW_OVERLAY_LAYER"] = "IG_LAYER_OVERLAY"
        display.root["IG_VIEW_OVERLAY_VIEW"] = [0.0, 0.0, 1.0, 0.0]
        display.root["IG_VIEW_DESKTOP_LAYER"] = "IG_LAYER_DESKTOP"
        display.root["IG_VIEW_DESKTOP_VIEW"] = [0.0, 0.0, 1.0, 0.0]
        display.root["IG_VIEW_ROOT_LAYER"] = "IG_LAYER_ROOT"
        display.root["IG_VIEW_ROOT_VIEW"] = [0.0, 0.0, 1.0, 0.0]
        display.root["IG_VIEWS"] = ["IG_VIEW_ROOT", "IG_VIEW_DESKTOP", "IG_VIEW_OVERLAY", "IG_VIEW_MENU"]

        with pkg_resources.resource_stream("glass_theme", "shader_window_geometry.glsl") as f:
            display.root["IG_SHADER_DEFAULT_GEOMETRY"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_window_vertex.glsl") as f:
            display.root["IG_SHADER_DEFAULT_VERTEX"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_window_fragment.glsl") as f:
            display.root["IG_SHADER_DEFAULT_FRAGMENT"] = f.read()

        with pkg_resources.resource_stream("glass_theme", "shader_root_geometry.glsl") as f:
            display.root["IG_SHADER_ROOT_GEOMETRY"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_root_vertex.glsl") as f:
            display.root["IG_SHADER_ROOT_VERTEX"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_root_fragment.glsl") as f:
            display.root["IG_SHADER_ROOT_FRAGMENT"] = f.read()

        display.root["IG_SHADERS"] = ["IG_SHADER_ROOT", "IG_SHADER_DEFAULT"]

        display.root["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINES"
        with pkg_resources.resource_stream("glass_theme", "coastline50.geojson") as f:
            display.root["IG_COASTLINE"] = linestrings2texture(f)

        # display.root["IG_DRAW_TYPE"] = "IG_DRAW_TYPE_LINE_STRIP"
        # display.root["SERPENT"] = [
        #     c for s in range(9, 0, -1) for c in [-.1 * s, -.1 * (s + 1),
        #                                          -.1 * s, .1 *s,
        #                                          .1 * s, .1 * s,
        #                                          .1 * s, -.1 * s]]
        
        InfiniteGlass.DEBUG("init", "Theme started\n")

        display.sync()
