import InfiniteGlass

class Locker(object):
    def __init__(self, display, config, **kw):
        self.display = display
        self.config = config
        for k, v in kw.items():
            setattr(self, k, v)

        self.action_runner = InfiniteGlass.action.ActionRunner(
            self.display)
            
        @display.root.on(mask="StructureNotifyMask", client_type="IG_SCREEN_LOCK")
        def ClientMessage(win, ev):
            print("Received screen lock message")
            self.lock_with_animation()

    def lock_with_animation(self):
        animation = self.config.get("animation", {}).get("lock")
        if animation is not None:
            self.action_runner.run(animation)
            args = next(iter(animation.values()))
            if args and "timeframe" in args:
                self.display.mainloop.add_timeout(
                    args["timeframe"],
                    lambda timestamp: self.lock())
                return
        self.lock()
            
    def lock(self):
        raise NotImplemented

    def unlocked(self):
        self.display.root.send(
            self.display.root,
            "IG_SCREEN_UNLOCKED",
            0)
        self.display.flush()
        animation = self.config.get("animation", {}).get("unlock")
        if animation is not None:
            self.action_runner.run(animation)
