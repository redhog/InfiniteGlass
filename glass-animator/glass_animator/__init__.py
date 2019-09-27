import InfiniteGlass
import time
import select
import array
import traceback

debug_aninmation = False

animations = []

def animate(display):
    while True:
        if display.pending_events():
            return
        r, w, e = select.select([display], [], [], 0.01)
        if r or w or e:
            return
        for animation in animations:
            try:
                next(animation)
            except StopIteration:
                animations.remove(animation)
            except Exception as e:
                print("Animation failed: ", e)
                traceback.print_exc()

def animate_property(display, window, atom, timeframe):
    src = window[atom]
    if not isinstance(src, (tuple, list, array.array)): src = [src]
    dst = window[atom + "_ANIMATE"]
    if not isinstance(dst, (tuple, list, array.array)): dst = [dst]
    isint = isinstance(src[0], int)
    values = list(zip(src, dst))
    start = time.time()
    tick = [0]

    if debug_aninmation: print("ANIMATE %s.%s=%s...%s / %ss" % (window.__window__(), atom, src, dst, timeframe))

    def animationfn():
        while True:
            tick[0] += 1
            current = time.time()
            progress = (current - start) / timeframe
            if progress > 1.:
                window[atom] = dst
                animations.remove(animation)
                if debug_aninmation: print("SET FINAL %s.%s=%s" % (window.__window__(), atom, dst))
            else:
                res = [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]
                if isint:
                    res = [int(item) for item in res]
                window[atom] = res
                if debug_aninmation and tick[0] % 100 == 0:
                    print("SET %s.%s=%s" % (window.__window__(), atom, [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]))
            display.flush()
            display.sync()
            yield
    animation = iter(animationfn())
    animations.append(animation)

def animate_geometry(display, window, timeframe):
    src = window.get_geometry()
    src = [src.x, src.y, src.width, src.height]
    dst = window["__GEOMETRY__ANIMATE"]
    if not isinstance(dst, (tuple, list, array.array)): dst = [dst]
    values = list(zip(src, dst))
    start = time.time()
    tick = [0]

    print("ANIMATE [%s]=%s...%s / %ss" % (window.__window__(), src, dst, timeframe))

    def animationfn():
        while True:
            tick[0] += 1
            current = time.time()
            progress = (current - start) / timeframe
            if progress > 1.:
                current_values = dst
                animations.remove(animation)
                print("SET FINAL [%s]=%s" % (window.__window__(), dst))
            else:
                current_values = [int(progress * dstval + (1.-progress) * srcval) for srcval, dstval in values]
                if tick[0] % 100 == 0:
                    print("SET [%s]=%s" % (window.__window__(), [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]))
            window.configure(x=current_values[0], y=current_values[1], width=current_values[2], height=current_values[3])
            display.flush()
            display.sync()
            yield
    animation = iter(animationfn())
    animations.append(animation)

def main(*arg, **kw):
    with InfiniteGlass.Display() as display:
        @display.mainloop.add_interval(0.1)
        def animator(timestamp, idx):
            animate(display)

        w = display.root.create_window(map=False)
        display.root["IG_ANIMATE"] = w

        @w.on(client_type="IG_ANIMATE", mask="PropertyChangeMask")
        def ClientMessage(win, event):
            window, atom, timeframe = event.parse("WINDOW", "ATOM", "FLOAT")

            if atom == "__GEOMETRY__":
                animate_geometry(display, window, timeframe)
            else:
                animate_property(display, window, atom, timeframe)

        print("Animator started")
