import InfiniteGlass
import json
import Xlib.X
import sys
from . import base
from . import default

class Base(base.ThemeBase):
    theme = default.Theme

class NoSplash(Base):
    def activate(self):
        base.ThemeBase.activate(self)
        self.theme.activate()
        
class SplashAnimation(Base):
    root_IG_WORLD_COORDS: [70., 18.]

    def activate(self):
        base.ThemeBase.activate(self)
        self.theme.activate()

        @self.display.root.require("IG_WORLD_SPLASH_WINDOWS")
        def with_splash(w, v):
            action_runner = InfiniteGlass.action.ActionRunner(
                self.display
            ).run({
                "splash_unlock": {}})
