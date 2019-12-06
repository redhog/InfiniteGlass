from .. import mode as modemod

def ifnowin(self, event, mode=None):
    if not self.window or self.window == self.display.root:
        modemod.pop(self.display)
        if mode is not None:
            modemod.push_by_name(self.display, mode, first_event=self.first_event, last_event=self.last_event)

