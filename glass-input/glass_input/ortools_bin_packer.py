# Requires: pip install ortools
from ortools.sat.python import cp_model
import numpy as np
from fractions import Fraction
from math import gcd
from functools import reduce

def place_windows_int(screen_w, screen_h,
                      all_rects,
                      move_weight=None, # 1
                      zoom_weight=None, # 2
                      resize_weight=None, # 5
                      time_limit_seconds=1.0):
    """
    Place windows on a screen with non-overlap, allowing movement/resizing of existing windows.

    screen_w, screen_h: screen size in pixels (ints)
    windows: list of windows, each a dict:
        {
           'id': 'w1',
           'x': current_x, # optional current position
           'y': current_y, # optional current position
           'w': current_w,
           'h': current_h,
        }
    Updates all_rects to have new members "fit" with {x, y, w, h}
    Returns: True if success
    """
    model = cp_model.CpModel()
    
    vars_x = {}
    vars_y = {}
    vars_w = {}
    vars_h = {}
    x_intervals = []
    y_intervals = []
    penalties = []

    screen_w_max = screen_w
    screen_h_max = screen_h
    screen_w_min = screen_w
    screen_h_min = screen_h
    if zoom_weight is not None:
        screen_w_min = 0
        screen_h_min = 0
        screen_w_max *= 10
        screen_h_max *= 10
        
    screen_w_var = model.NewIntVar(screen_w_min, screen_w_max, f"screen_w")
    screen_h_var = model.NewIntVar(screen_h_min, screen_h_max, f"screen_h")
    
    if zoom_weight is not None:
        model.Add(screen_h_var * screen_w == screen_w_var * screen_h)
        penalties.append((screen_w_var, zoom_weight))        
        
    for win in all_rects:
        wid = win['id']

        vars_w[wid] = w_var = model.NewIntVar(0, screen_w_max, f"w_{wid}")
        vars_h[wid] = h_var = model.NewIntVar(0, screen_h_max, f"h_{wid}")
        vars_x[wid] = x_var = model.NewIntVar(0, screen_w_max, f"x_{wid}")
        vars_y[wid] = y_var = model.NewIntVar(0, screen_h_max, f"y_{wid}")
        x_end = model.NewIntVar(0, screen_w, f"xend_{wid}")
        y_end = model.NewIntVar(0, screen_h, f"yend_{wid}")
        
        model.Add(x_var + w_var <= screen_w_var)
        model.Add(y_var + h_var <= screen_h_var)

        x_interval = model.NewIntervalVar(x_var, w_var, x_end, f"xint_{wid}")
        y_interval = model.NewIntervalVar(y_var, h_var, y_end, f"yint_{wid}")
        x_intervals.append(x_interval)
        y_intervals.append(y_interval)

        if move_weight is None:
            if "x" in win:
                model.Add(x_var == win["x"])
            if "y" in win:
                model.Add(y_var == win["y"] - win["h"]) # Top left vs bottom left
        else:
            if "x" in win:
                dx = model.NewIntVar(-screen_w, screen_w, f"dx_{wid}")
                model.Add(dx == vars_x[wid] - win['x'])
                abs_dx = model.NewIntVar(0, screen_w, f"abs_dx_{wid}")
                model.AddAbsEquality(abs_dx, dx)
                penalties.append((abs_dx, move_weight))
            if "y" in win:
                dy = model.NewIntVar(-screen_h, screen_h, f"dy_{wid}")
                model.Add(dy == vars_y[wid] - (win['y'] - win['h'])) # Top left vs bottom left
                abs_dy = model.NewIntVar(0, screen_h, f"abs_dy_{wid}")
                model.AddAbsEquality(abs_dy, dy)
                penalties.append((abs_dy, move_weight))            
        
        if resize_weight is None:
            model.Add(w_var == win['w'])
            model.Add(h_var == win['h'])
        else:
            dw = model.NewIntVar(-screen_w_max, screen_w_max, f"dw_{wid}")
            model.Add(dw == w_var - win['w'])
            abs_dw = model.NewIntVar(0, screen_w_max, f"abs_dw_{wid}")
            model.AddAbsEquality(abs_dw, dw)

            dh = model.NewIntVar(-screen_h_max, screen_h_max, f"dh_{wid}")
            model.Add(dh == h_var - win['h'])
            abs_dh = model.NewIntVar(0, screen_h_max, f"abs_dh_{wid}")
            model.AddAbsEquality(abs_dh, dh)
            
            resize_pen = model.NewIntVar(0, screen_w_max*2 + screen_h_max*2, f"resize_pen_{wid}")
            model.Add(resize_pen == abs_dw + abs_dh)
            penalties.append((resize_pen, resize_weight))

    model.AddNoOverlap2D(x_intervals, y_intervals)
    
    model.Minimize(sum((var * w for var, w in penalties)))

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = time_limit_seconds
    solver.parameters.num_search_workers = 8  # tune to your machine
#    solver.parameters.maximize = False

    result = solver.Solve(model)

    if result == cp_model.OPTIMAL or result == cp_model.FEASIBLE:
        out = {}
        for win in all_rects:
            wid = win['id']
            win['fit'] = {"x": solver.Value(vars_x[wid]),
                          "y": solver.Value(vars_y[wid]) + solver.Value(vars_h[wid]), # Top left vs bottom left
                          "w": solver.Value(vars_w[wid]),
                          "h": solver.Value(vars_h[wid])}
        return True
    else:
        return False

def lcm(a, b): return a * b // gcd(a, b)

def find_scale(values):
    denominators = [Fraction(v).limit_denominator().denominator for v in values]
    return reduce(lcm, denominators, 1)

def place_windows(screen_w, screen_h, windows, **kw):
    """
    Scale all floats to integers using the exact binary representation,
    run a bin-packing function, then scale back down. Uses numpy for speed.
    """
    keys = ("x", "y", "w", "h")
    values = [screen_w, screen_h]
    for win in windows:
        for k in keys:
            if k in win:
                values.append(win[k])
                
    if not values:
        return place_windows_int(screen_w, screen_h, windows, **kw)

    scale_factor = find_scale(values)                  
    
    int_windows = [
        {"orig": win,
         "id": win["id"],
         **{k: int(round(win[k] * scale_factor))
            for k in keys if k in win}}
        for win in windows]

    int_screen_w = int(round(screen_w * scale_factor))
    int_screen_h = int(round(screen_h * scale_factor))

    res = place_windows_int(int_screen_w, int_screen_h, int_windows, **kw)
    if res:
        for win in int_windows:
            win["orig"]["fit"] = {k: win["fit"][k] / scale_factor
                                  for k in keys
                                  if k in win["fit"]}

    return res

class CpPacker(object):
  def fit(self, blocks, view_width, view_height, **kw):
      place_windows(view_width, view_height, blocks, **kw)
    
# Example usage:
if __name__ == "__main__":
    screen_w, screen_h = 1500, 1500
    existing = [
        {'id':'w1','x':0,'y':0,'w':800,'h':600},
        {'id':'w2','x':810,'y':0,'w':400,'h':300},
        {'id':'w3','x':0,'y':610,'w':500,'h':300},
        {'id':'w4','x':510,'y':610,'w':600,'h':400},
    ]
    for w in existing:
        w["min_w"] = w["max_w"] = w["w"]
        w["min_h"] = w["max_h"] = w["h"]
        
    new_w = {'id':'new','min_w':200,'min_h':100,'max_w':200,'max_h':100}
    layout = place_windows_int(screen_w, screen_h, existing, new_w, move_weight=1, resize_weight=10, time_limit_seconds=0.5)
    print(layout)
