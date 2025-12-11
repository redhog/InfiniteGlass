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

        action_runner = InfiniteGlass.action.ActionRunner(
            self.display
        ).run({
            "splash_unlock": {}})
