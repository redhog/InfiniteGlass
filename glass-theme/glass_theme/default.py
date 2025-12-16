import InfiniteGlass
from . import base

class Theme(base.ThemeBase):
    def activate(self):
        base.ThemeBase.activate(self)

    shaders_path = "resource://glass_theme/shaders"
    
    shader_DEFAULT = None
    shader_DECORATION = None
    shader_ROOT = "root-fractal-julia"
    shader_SPLASH = None
    shader_SPLASH_BACKGROUND = None

    root_IG_SHADER = "IG_SHADER_ROOT"
    
    root_IG_VIEW_MENU_LAYER = "IG_LAYER_MENU"
    root_IG_VIEW_MENU_VIEW = [0.0, 0.0, 1.0, "eq://$.root.aspect_ratio"]
    root_IG_VIEW_OVERLAY_LAYER = "IG_LAYER_OVERLAY"
    root_IG_VIEW_OVERLAY_VIEW = [0.0, 0.0, 1.0, "eq://$.root.aspect_ratio"]
    root_IG_VIEW_DESKTOP_LAYER = ["IG_LAYER_ISLAND", "IG_LAYER_DESKTOP"]
    root_IG_VIEW_DESKTOP_VIEW = [0.0, 0.0, 1.0, "eq://$.root.aspect_ratio"]

    root_IG_VIEW_ROOT_LAYER = "IG_LAYER_ROOT"
    root_IG_VIEW_ROOT_VIEW = [0.0, 0.0, 1.0, "eq://$.root.aspect_ratio"]

    root_IG_VIEW_SPLASH_LAYER = "IG_LAYER_SPLASH"
    root_IG_VIEW_SPLASH_VIEW = [0.0, 0.0, 1.0, "eq://$.root.aspect_ratio"]
    root_IG_VIEW_SPLASH_BACKGROUND_LAYER = "IG_LAYER_SPLASH_BACKGROUND"
    root_IG_VIEW_SPLASH_BACKGROUND_VIEW = [0.0, 0.0, 1.0, "eq://$.root.aspect_ratio"]

    
    root_IG_VIEWS = ["IG_VIEW_ROOT",
                     "IG_VIEW_DESKTOP",
                     "IG_VIEW_OVERLAY",
                     "IG_VIEW_SPLASH_BACKGROUND",
                     "IG_VIEW_SPLASH",
                     "IG_VIEW_MENU"]
    
    root_IG_COLOR_TRANSFORM = 1
    
    
    define_EDGE_HINT_WIDTH = 4
    define_EDGE_HINT_COLOR = "vec4(0., 0., 0., 0.3)"
    
    define_margin_left = 6
    define_margin_right = 6
    define_margin_top = 6
    define_margin_bottom = 6

    define_ICON_CUTOFF_1 = .4
    define_ICON_CUTOFF_2 = .3

    define_DECORATION_CUTOFF = 0.25

    define_DECORATION_MOUSEDIST_2 = 150.0
    define_DECORATION_MOUSEDIST_1 = 100.0

    define_BORDER_COLOR_1 = "vec4(0., 0., 0., 0.)"
    define_BORDER_COLOR_2 = "vec4(0., 0., 0., 0.)"
    define_BORDER_COLOR_3 = "vec4(0., 0., 0., 0.)"
    define_BORDER_COLOR_4 = "vec4(0., 0., 0., 0.)"
    define_BORDER_COLOR_5 = "vec4(0., 0., 0., 0.)"
    define_BORDER_COLOR_6 = "vec4(0., 0., 0., 1.)"

    define_BORDER_ACTIVE_COLOR_1 = "vec4(0., 0., 0., 0.)"
    define_BORDER_ACTIVE_COLOR_2 = "vec4(0., 0., 0., 0.)"
    define_BORDER_ACTIVE_COLOR_3 = "vec4(0., 0., 0., 1.)"
    define_BORDER_ACTIVE_COLOR_4 = "vec4(1., 1., 1., 1.)"
    define_BORDER_ACTIVE_COLOR_5 = "vec4(0., 0., 0., 1.)"
    define_BORDER_ACTIVE_COLOR_6 = "vec4(0., 0., 0., 1.)"

    # See glass-theme/glass_theme/shaders/root/fragment.glsl for list of possible values
    define_BACKGROUND_TYPE = 1
    # 200 gives a much better picture, but is super slow in software...
    define_FRACTAL_PRECISION = 10

    define_IG_COLOR_TRANSFORM_DEFAULT = 0;
    define_IG_COLOR_TRANSFORM_BORDER_DEFAULT = 0;
    
    define_COLOR_TRANSFORM_1 = ("transpose(mat4(" +
                                "0.7, 0.0, 0.0, 0.3," +
                                "0.0, 0.7, 0.0, 0.3," +
                                "0.0, 0.0, 0.7, 0.3," +
                                "0.0, 0.0, 0.0, 1.0" +
                                "))")
    define_COLOR_TRANSFORM_2 = ("transpose(mat4(" +
                                "1.0, 0.0, 0.0, 0.0," +
                                "0.0, 1.0, 0.0, 0.0," +
                                "0.0, 0.0, 1.0, 0.0," +
                                "0.0, 0.0, 0.0, 1.0" +
                                "))")
    define_COLOR_TRANSFORM_3 = ("transpose(mat4(" +
                                "1.0, 0.0, 0.0, 0.0," +
                                "0.0, 1.0, 0.0, 0.0," +
                                "0.0, 0.0, 1.0, 0.0," +
                                "0.0, 0.0, 0.0, 1.0" +
                                "))")
    
