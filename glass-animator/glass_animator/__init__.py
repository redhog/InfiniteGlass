import InfiniteGlass
import time
import select
import array
import traceback
import numpy as np

animations = {}

def animate(display):
    for animationid, animation in list(animations.items()):
        try:
            next(animation)
        except StopIteration:
            try:
                del animations[animationid]
            except:
                pass
        except Exception as e:
            print("Animation failed: ", e)
            traceback.print_exc()
            try:
                del animations[animationid]
            except:
                pass
                
def start_animation(animationid, animation):
    animations[animationid] = animation

def animate_anything(display, **kw):
    window = kw.get("window", None)
    atom = kw.get("atom", None)
    if window and atom:
        dst = window.get(atom + "_ANIMATE", None)
        if dst is not None:
            if isinstance(dst, dict):
                kw.update(dst)
            else:
                kw["dst"] = dst
    if atom == "__GEOMETRY__":
        return animate_geometry(display, **kw)
    elif "steps" in kw:
        return animate_sequence(display, **kw)
    elif "tasks" in kw:
        return animate_parallel(display, **kw)
    elif atom is not None:
        return animate_property(display, **kw)
    else:
        return animate_nothing(display, **kw)

def animate_parallel(display, timeframe=0.0, tasks=None, **kw):
    if timeframe == 0.0:
        factor = 1
    else:
        total_time = max(task[2] for task in tasks)
        factor = timeframe / total_time

    active_tasks = {}
    for task in tasks:
        task = dict(task)
        if "timeframe" in task:
            task["timeframe"] *= factor
        active_task = animate_anything(display, **task)
        active_tasks[id(active_task)] = active_task

    while active_tasks:
        for task_id, active_task in list(active_tasks.items()):
            try:
                yield next(active_task)
            except StopIteration:
                del active_tasks[task_id]
    
def animate_sequence(display, timeframe=0.0, steps=None, **kw):
    if timeframe == 0.0:
        factor = 1
    else:
        total_time = sum(step[2] for step in steps)
        factor = timeframe / total_time

    for step in steps:
        step = dict(step)
        if "timeframe" in step:
            step["timeframe"] *= factor
        for part in animate_anything(display, **step):
            yield part

def animate_nothing(display, timeframe, **kw):
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

            
def animate_property(display, window, atom, timeframe=0.0, src=None, dst=None, easing="Linear", **kw):
    easing = easings[easing]
    if not isinstance(dst, (tuple, list, array.array)): dst = [dst]
    if timeframe == 0.0:
        InfiniteGlass.DEBUG("begin", "ANIMATE %s.%s=%s\n" % (window.__window__(), atom, dst))
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
                pos = easing(progress)
                res = [pos * dstval + (1.-pos) * srcval for srcval, dstval in values]
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

def animate_geometry(display, window, timeframe, src=None, dst=None, easing="Linear", **kw):
    easing = easings[easing]
    if src is None:
        src = window.get_geometry()
        src = [src.x, src.y, src.width, src.height]
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
                pos = easing(progress)
                current_values = [int(pos * dstval + (1.-pos) * srcval) for srcval, dstval in values]
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

# Thanks to Peter Much, see https://github.com/PMunch/SDLGamelib/blob/master/gamelib/tween.nim#L163
# For pictures of how these look, check out https://easings.net/en
easings = {
  "Linear": lambda t: t,
  "InSine": lambda t: 1-sin(np.pi/2+t*np.pi/2),
  "OutSine": lambda t: sin(t*np.pi/2),
  "InOutSine": lambda t: (1-sin(np.pi/2+t*np.pi))/2,
  "InQuad": lambda t: pow(t,2),
  "OutQuad": lambda t: t*(2-t),
  "InOutQuad": lambda t: (2*t*t if t<0.5 else -1+(4-2*t)*t),
  "InCubic": lambda t: pow(t,3),
  "OutCubic": lambda t: pow(t-1,3)+1,
  "InOutCubic": lambda t: (4*pow(t,3) if t<0.5 else (t-1)*pow(2*t-2,2)+1),
  "InQuart": lambda t: pow(t,4),
  "OutQuart": lambda t: 1-pow(t-1,4),
  "InOutQuart": lambda t: (8*pow(t,4) if t<0.5 else 1-8*pow(t-1,4)),
  "InQuint": lambda t: pow(t,5),
  "OutQuint": lambda t: 1+pow(t-1,5),
  "InOutQuint": lambda t: (16*pow(t,5) if t<0.5 else 1+16*pow(t-1,5)),
  "InExpo": lambda t: pow(2,10*(t/1-1)),
  "OutExpo": lambda t: 1*(-pow(2,-10*t/1)+1),
  "InOutExpo": lambda t: (0.5*pow(2,(10 * (t*2 - 1))) if t<0.5 else 0.5*(-pow(2, -10*(t*2-1))+2)),
  "InCirc": lambda t: -1*(sqrt(1 - t * t) - 1),
  "OutCirc": lambda t: 1*sqrt(1 - (t-1).pow(2)),
  "InOutCirc": lambda t: (-0.5*(sqrt(1 - pow(t*2,2)) - 1) if t < 0.5 else 0.5*(sqrt(1 - pow(t*2-2,2)) + 1)),
  "InBack": lambda t: t * t * (2.70158 * t - 1.70158),
  "OutBack": lambda t: 1 - (1-t) * (1-t) * (2.70158 * (1-t) - 1.70158),
  "InOutBack": lambda t: (0.5*(4*t*t*(3.5949095 * t*2 - 2.5949095)) if t<0.5 else 0.5 * ((t*2 - 2).pow(2) * ((4.9572369875) * (t*2-2) + 3.9572369875) + 2))
}

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
            start_animation(animationid, animate_anything(display, window=window, atom=atom, timeframe=timeframe))

        InfiniteGlass.DEBUG("init", "Animator started\n")
