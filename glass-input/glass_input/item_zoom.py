from . import mode

class ItemZoomMode(mode.Mode):
    def enter(self):
        mode.Mode.enter(self)
        if not self.window or self.window == self.display.root:
            return False
        return True
