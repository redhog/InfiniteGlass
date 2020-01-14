import InfiniteGlass
import time
import select
import array
import traceback

animations = {}

def animate(display):
    while True:
        if display.pending_events():
            return
        r, w, e = select.select([display], [], [], 0.01)
        if r or w or e:
            return
        for animationid, animation in list(animations.items()):
            try:
                next(animation)
            except StopIteration:
                del animations[animationid]
            except Exception as e:
                print("Animation failed: ", e)
                traceback.print_exc()

def start_animation(animationid, animation):
    animations[animationid] = animation

def animate_anything(display, window, atom, timeframe):
    if atom == "__GEOMETRY__":
        return animate_geometry(display, window, timeframe)
    elif atom.endswith("_SEQUENCE"):
        return animate_sequence(display, window, atom, timeframe)
    else:
        return animate_property(display, window, atom, timeframe)
    
def animate_sequence(display, window, atom, timeframe):
    sequence = window[atom]

    total_time = sum(step[2] for step in sequence["steps"])
    factor = timeframe / total_time

    for step_win, step_atom, step_timeframe in sequence["steps"] :
        step_win = display.create_resource_object("window", step_win)
        for part in animate_anything(display, step_win, step_atom, factor * step_timeframe):
            yield part

def animate_property(display, window, atom, timeframe):
    src = window[atom]
    if not isinstance(src, (tuple, list, array.array)): src = [src]
    dst = window[atom + "_ANIMATE"]
    if not isinstance(dst, (tuple, list, array.array)): dst = [dst]
    isint = isinstance(src[0], int)
    values = list(zip(src, dst))
    start = time.time()
    tick = [0]
    
    InfiniteGlass.DEBUG("begin", "ANIMATE %s.%s=%s...%s / %ss\n" % (window.__window__(), atom, src, dst, timeframe))

    def animationfn():
        while True:
            tick[0] += 1
            current = time.time()
            progress = (current - start) / timeframe
            if progress > 1.:
                window[atom] = dst
                InfiniteGlass.DEBUG("final", "SET FINAL %s.%s=%s\n" % (window.__window__(), atom, dst))
            else:
                res = [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]
                if isint:
                    res = [int(item) for item in res]
                window[atom] = res
                InfiniteGlass.DEBUG("transition", "SET %s.%s=%s\n" % (window.__window__(), atom, [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]))
            display.flush()
            display.sync()
            if progress > 1.:
                return
            yield
    return animationfn()

def animate_geometry(display, window, timeframe):
    src = window.get_geometry()
    src = [src.x, src.y, src.width, src.height]
    dst = window["__GEOMETRY__ANIMATE"]
    if not isinstance(dst, (tuple, list, array.array)): dst = [dst]
    values = list(zip(src, dst))
    start = time.time()
    tick = [0]
    
    InfiniteGlass.DEBUG("begin", "ANIMATE [%s]=%s...%s / %ss\n" % (window.__window__(), src, dst, timeframe))

    def animationfn():
        while True:
            tick[0] += 1
            current = time.time()
            progress = (current - start) / timeframe
            if progress > 1.:
                current_values = dst
                InfiniteGlass.DEBUG("final", "SET FINAL [%s]=%s\n" % (window.__window__(), dst))
            else:
                current_values = [int(progress * dstval + (1.-progress) * srcval) for srcval, dstval in values]
                if tick[0] % 100 == 0:
                    InfiniteGlass.DEBUG(
                        "transition", "SET [%s]=%s\n" % (window.__window__(), [progress * dstval + (1.-progress) * srcval for srcval, dstval in values]))
            window.configure(x=current_values[0], y=current_values[1], width=current_values[2], height=current_values[3])
            display.flush()
            display.sync()
            if progress > 1.:
                return
            yield
    return animationfn()
    
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
            animationid = (window.__window__(), atom)
            start_animation(animationid, animate_anything(display, window, atom, timeframe))

        InfiniteGlass.DEBUG("init", "Animator started\n")
