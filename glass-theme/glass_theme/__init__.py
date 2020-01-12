import InfiniteGlass
import pkg_resources

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

        with pkg_resources.resource_stream("glass_theme", "shader_window_geometry.glsl") as f:
            display.root["IG_SHADER_TEST_GEOMETRY"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_window_vertex.glsl") as f:
            display.root["IG_SHADER_TEST_VERTEX"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_window_fragment_test.glsl") as f:
            display.root["IG_SHADER_TEST_FRAGMENT"] = f.read()

        with pkg_resources.resource_stream("glass_theme", "shader_root_geometry.glsl") as f:
            display.root["IG_SHADER_ROOT_GEOMETRY"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_root_vertex.glsl") as f:
            display.root["IG_SHADER_ROOT_VERTEX"] = f.read()
        with pkg_resources.resource_stream("glass_theme", "shader_root_fragment.glsl") as f:
            display.root["IG_SHADER_ROOT_FRAGMENT"] = f.read()

        display.root["IG_SHADERS"] = ["IG_SHADER_ROOT", "IG_SHADER_DEFAULT", "IG_SHADER_TEST"]

        InfiniteGlass.DEBUG("init", "Theme started\n")

        display.sync()
