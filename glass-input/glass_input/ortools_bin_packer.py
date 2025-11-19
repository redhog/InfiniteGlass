# Requires: pip install ortools
from ortools.sat.python import cp_model
import numpy as np
from fractions import Fraction
from math import gcd
from functools import reduce

def add_non_overlap_interval_vs_fixed(model,
                                      xint, yint,
                                      fx, fy, fw, fh,
                                      name_prefix):
    """
    Enforce that a movable interval (xint, yint) does NOT overlap
    a fixed rectangle (fx, fy, fw, fh).

    (fx,fy) = fixed top-left coordinate
    fw,fh  = fixed width/height
    """

    left  = model.NewBoolVar(name_prefix + "_left")
    right = model.NewBoolVar(name_prefix + "_right")
    above = model.NewBoolVar(name_prefix + "_above")
    below = model.NewBoolVar(name_prefix + "_below")

    model.Add(xint.StartExpr() + xint.SizeExpr() <= fx).OnlyEnforceIf(left)
    model.Add(xint.StartExpr() + xint.SizeExpr() >  fx).OnlyEnforceIf(left.Not())

    model.Add(xint.StartExpr() >= fx + fw).OnlyEnforceIf(right)
    model.Add(xint.StartExpr() <  fx + fw).OnlyEnforceIf(right.Not())

    model.Add(yint.StartExpr() + yint.SizeExpr() <= fy).OnlyEnforceIf(above)
    model.Add(yint.StartExpr() + yint.SizeExpr() >  fy).OnlyEnforceIf(above.Not())

    model.Add(yint.StartExpr() >= fy + fh).OnlyEnforceIf(below)
    model.Add(yint.StartExpr() <  fy + fh).OnlyEnforceIf(below.Not())

    model.AddBoolOr([left, right, above, below])

def pack_int(windows, screen_w, screen_h,
             move_weight=1,
             zoom_weight=1,
             resize_weight=None,
             time_limit_seconds=1.0):

    model = cp_model.CpModel()

    vars_x, vars_y, vars_w, vars_h = {}, {}, {}, {}
    movable_x_intervals = []
    movable_y_intervals = []
    movable_ids = []
    fixed_windows = []
    penalties = []

    screen_w_min = screen_w if zoom_weight is None else 0
    screen_h_min = screen_h if zoom_weight is None else 0
    screen_w_max = screen_w if zoom_weight is None else screen_w * 10
    screen_h_max = screen_h if zoom_weight is None else screen_h * 10

    screen_w_var = model.NewIntVar(screen_w_min, screen_w_max, "screen_w")
    screen_h_var = model.NewIntVar(screen_h_min, screen_h_max, "screen_h")

    if zoom_weight is not None:
        model.Add(screen_h_var * screen_w == screen_w_var * screen_h)
        penalties.append((screen_w_var, zoom_weight))

    for wid, win in enumerate(windows):

        win_move_weight = win.get("move_weight", move_weight)
        win_resize_weight = win.get("resize_weight", resize_weight)

        if win_move_weight is None and "x" in win and "y" in win:
            vars_x[wid] = win["x"]
            vars_y[wid] = win["y"] - win["h"]
            vars_w[wid] = win["w"]
            vars_h[wid] = win["h"]

            fixed_windows.append((wid, win))
        else:
            w_var = model.NewIntVar(0, screen_w_max, f"w_{wid}")
            h_var = model.NewIntVar(0, screen_h_max, f"h_{wid}")
            x_var = model.NewIntVar(0, screen_w_max, f"x_{wid}")
            y_var = model.NewIntVar(0, screen_h_max, f"y_{wid}")

            vars_w[wid] = w_var
            vars_h[wid] = h_var
            vars_x[wid] = x_var
            vars_y[wid] = y_var

            x_end = model.NewIntVar(0, screen_w_max, f"xend_{wid}")
            y_end = model.NewIntVar(0, screen_h_max, f"yend_{wid}")

            model.Add(x_var + w_var <= screen_w_var)
            model.Add(y_var + h_var <= screen_h_var)

            xint = model.NewIntervalVar(x_var, w_var, x_end, f"xint_{wid}")
            yint = model.NewIntervalVar(y_var, h_var, y_end, f"yint_{wid}")

            movable_x_intervals.append(xint)
            movable_y_intervals.append(yint)
            movable_ids.append(wid)

            if "x" in win:
                dx = model.NewIntVar(-screen_w_max, screen_w_max, f"dx_{wid}")
                model.Add(dx == x_var - win["x"])
                adx = model.NewIntVar(0, screen_w_max, f"adx_{wid}")
                model.AddAbsEquality(adx, dx)
                penalties.append((adx, win_move_weight))

            if "y" in win:
                dy = model.NewIntVar(-screen_h_max, screen_h_max, f"dy_{wid}")
                model.Add(dy == y_var - (win["y"] - win["h"]))
                ady = model.NewIntVar(0, screen_h_max, f"ady_{wid}")
                model.AddAbsEquality(ady, dy)
                penalties.append((ady, win_move_weight))

            if win_resize_weight is None:
                model.Add(w_var == win["w"])
                model.Add(h_var == win["h"])
            else:
                dw = model.NewIntVar(-screen_w_max, screen_w_max, f"dw_{wid}")
                model.Add(dw == w_var - win["w"])
                adw = model.NewIntVar(0, screen_w_max, f"adw_{wid}")
                model.AddAbsEquality(adw, dw)

                dh = model.NewIntVar(-screen_h_max, screen_h_max, f"dh_{wid}")
                model.Add(dh == h_var - win["h"])
                adh = model.NewIntVar(0, screen_h_max, f"adh_{wid}")
                model.AddAbsEquality(adh, dh)

                rp = model.NewIntVar(0, screen_w_max * 2 + screen_h_max * 2,
                                      f"resize_{wid}")
                model.Add(rp == adw + adh)
                penalties.append((rp, win_resize_weight))

    if len(movable_x_intervals) > 1:
        model.AddNoOverlap2D(movable_x_intervals, movable_y_intervals)

    for interval_index, wid in enumerate(movable_ids):
        xint = movable_x_intervals[interval_index]
        yint = movable_y_intervals[interval_index]

        for fid, fwin in fixed_windows:
            add_non_overlap_interval_vs_fixed(
                model,
                xint, yint,
                vars_x[fid], vars_y[fid], vars_w[fid], vars_h[fid],
                name_prefix=f"mov_{wid}_vs_fix_{fid}"
            )
        
    model.Minimize(sum(var * w for var, w in penalties))

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = time_limit_seconds
    solver.parameters.num_search_workers = 8

    result = solver.Solve(model)
    if result not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        raise Exception("Unable to tile")

    for wid, win in enumerate(windows):
        if isinstance(vars_x[wid], int):
            # fixed window
            win["fit"] = {
                "x": vars_x[wid],
                "y": vars_y[wid] + vars_h[wid],
                "w": vars_w[wid],
                "h": vars_h[wid]
            }
        else:
            # movable window
            x = solver.Value(vars_x[wid])
            y = solver.Value(vars_y[wid])
            wv = solver.Value(vars_w[wid])
            hv = solver.Value(vars_h[wid])
            win["fit"] = {
                "x": x,
                "y": y + hv,
                "w": wv,
                "h": hv
            }

def pack(windows, screen_w, screen_h, **kw):
    """
    Scale all floats to integers using the exact binary representation,
    run a bin-packing function, then scale back down. Uses numpy for speed.
    """
    if not windows: return 

    xoffset = min((win["x"] for win in windows if "x" in win), default=0)
    yoffset = min((win["y"]-win["h"] for win in windows if "y" in win), default=0)
    xscale = 4906 / max((win.get("x", 0)+win["w"] for win in windows)) - xoffset
    yscale = 4096 / max((win["y"] if "y" in win else win["h"] for win in windows)) - yoffset
    
    int_windows = [
        {"orig": win,
         "w": int(round(win["w"] * xscale)),
         "h": int(round(win["h"] * yscale)),
         **({"x": int(round((win["x"]-xoffset) * xscale)),
             "y": int(round((win["y"]-yoffset) * yscale))}
            if ("x" in win and "y" in win) else {})}
        for win in windows]

    int_screen_w = int(round(screen_w * xscale))
    int_screen_h = int(round(screen_h * yscale))

    pack_int(int_windows, int_screen_w, int_screen_h, **kw)

    for win in int_windows:
        # Hack: If the packer didn't move the window, preserve the
        # exact original coordinate, not the rounded one.
        win["orig"]["fit"] = {
            "x": (win["fit"]["x"] / xscale + xoffset) if win["fit"]["x"] != win.get("x") else win["orig"]["x"],
            "y": (win["fit"]["y"] / yscale + yoffset) if win["fit"]["y"] != win.get("y") else win["orig"]["y"],
            "w": (win["fit"]["w"] / xscale) if win["fit"]["w"] != win.get("w") else win["orig"]["w"],
            "h": (win["fit"]["h"] / yscale) if win["fit"]["h"] != win.get("h") else win["orig"]["h"]}
