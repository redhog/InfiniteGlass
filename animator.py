import InfiniteGlass
import time
import select

animations = []

def animate(display):
    while True:
        r, w, e = select.select([display], [], [], 0.01)
        if r or w or e:
            return
        for animation in animations:
            next(animation)

    
with InfiniteGlass.Display() as display:
    next_event = display.next_event
    def new_next_event():
        animate(display)
        return next_event()
    display.next_event = new_next_event

    w = display.root.create_window(map=False)
    display.root["IG_ANIMATE"] = w

    @w.on(client_type="IG_ANIMATE", mask="PropertyChangeMask")
    def ClientMessage(win, event):
        window, atom, timeframe = event.parse("WINDOW", "ATOM", "FLOAT")

        src = window[atom]
        if not isinstance(src, (tuple, list)): src = [src]
        dst = window[atom + "_ANIMATE"]
        if not isinstance(dst, (tuple, list)): dst = [dst]
        values = list(zip(src, dst))
        start = time.time()
        tick = [0]

        print("ANIMATE %s.%s=%s...%s / %ss" % (window.__window__(), atom, src, dst, timeframe))
        
        def animationfn():
            while True:
                tick[0] += 1
                current = time.time()
                progress = (current - start) / timeframe
                if progress > 1.:
                    window[atom] = dst
                    animations.remove(animation)
                    print("SET FINAL %s.%s=%s" % (window.__window__(), atom, dst))
                else:
                    window[atom] = [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]
                    if tick[0] % 100 == 0:
                        print("SET %s.%s=%s" % (window.__window__(), atom, [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]))
                display.flush()
                display.sync()
                yield
        animation = iter(animationfn())
        animations.append(animation)

    print("ANIMATOR STARTED")
