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

def animate_anything(display, window, atom, timeframe=0.0, **kw):
    if atom is None:
        return animate_nothing(display, timeframe)
    elif atom == "__GEOMETRY__":
        return animate_geometry(display, window, timeframe, **kw)
    elif atom.endswith("_SEQUENCE"):
        return animate_sequence(display, window, atom, timeframe, **kw)
    else:
        return animate_property(display, window, atom, timeframe, **kw)
    
def animate_sequence(display, window, atom, timeframe=0.0):
    sequence = window[atom]

    if timeframe == 0.0:
        factor = 1
    else:
        total_time = sum(step[2] for step in sequence["steps"])
        factor = timeframe / total_time

    for step in sequence["steps"]:
        step = dict(step)
        step_win = step.pop("window", None)
        step_win = step_win and display.create_resource_object("window", step_win)
        step_atom = step.pop("atom", None)
        step_timeframe = factor * step.pop("timeframe", 0.0)
        for part in animate_anything(display, step_win, step_atom, step_timeframe, **step):
            yield part

def animate_nothing(display, timeframe):
    start = time.time()
    InfiniteGlass.DEBUG("begin", "ANIMATE NOTHING %ss\n" % (timeframe,))
    def animationfn():
        while True:
            current = time.time()
            progress = (current - start) / timeframe
            if progress > 1.:
                InfiniteGlass.DEBUG("final", "ANIMATION FINAL\n")
                return
            yield
    return animationfn()

            
def animate_property(display, window, atom, timeframe, src=None, dst=None):
    if dst is None:
        dst = window[atom + "_ANIMATE"]
    if not isinstance(dst, (tuple, list, array.array)): dst = [dst]
    if timeframe == 0.0:
        def animationfn():
            window[atom] = dst
            display.flush()
            yield
        return animationfn()
    if src is None:
        src = window[atom]
    if not isinstance(src, (tuple, list, array.array)): src = [src]
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

def animate_geometry(display, window, timeframe, src=None, dst=None):
    if src is None:
        src = window.get_geometry()
        src = [src.x, src.y, src.width, src.height]
    if dst is None:
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
