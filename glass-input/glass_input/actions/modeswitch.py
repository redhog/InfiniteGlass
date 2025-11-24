def ifnowin(self, mode=None, **kw):
    window = self.get_window(**kw)
    if not window or window == self.display.root:
        self.config.pop()
        if mode is not None:
            self.config.push_by_name(mode, first_event=self.first_event, last_event=self.last_event)
        else:
            raise Exception("No window selected, and no alternative mode to fall back to")
