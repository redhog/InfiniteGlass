import yaml
import os.path

config = None

NUMBERS = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"]
COMPATIBLE_EVENTS = (("KeyPress", "KeyRelease"), ("ButtonPress", "ButtonRelease", "MotionNotify"))
EVENT_TYPES = [event for group in COMPATIBLE_EVENTS for event in group]

def flatten_value(value, path=()):
    if isinstance(value, list):
        yield (path, ", ".join(res_item
                               for item in value
                               for key, res_item in flatten_value(item, path)))
    elif isinstance(value, str):
        if value == "pop":
            pass
        elif value in config["modes"]:
            sub_mode = config["modes"][value]
            if "keymap" in sub_mode:
                cls = sub_mode.get("class", "glass_input.mode.Mode")
                if cls not in ("glass_input.mode.Mode", "glass_input.grabbed.GrabbedMode"):
                    for (key, value) in flatten(sub_mode, path):
                        yield (key, "%s.%s" % (cls, value))
                else:
                    for res in flatten(sub_mode, path):
                        yield res
            else:
                yield (path, value)
        else:
            yield (path, value)
    elif isinstance(value, dict) and len(value) == 1:
        name = list(value.keys())[0]
        yield (path, (name, tuple("%s=%s" % (p, v) for (p, v) in value[name].items())))
    else:
        yield (path, value)

def flatten(mode, path=()):
    for key, value in mode["keymap"].items():
        key = path + (tuple(key.split(",")), )
        if not key: continue
        for res in flatten_value(value, key):
            yield res


def main_event(value):
    keys = [item for item in value if item.startswith("XK_") or item in NUMBERS]
    if keys: return keys[0]
    return None
            
def path_join(p1, p2):
    if not p1:
        return p2
    same = False
    for event_type in EVENT_TYPES:
        if event_type in p1[-1] and event_type in p2[0]:
            same = True
            break        
    if same:
        main1 = main_event(p2[0])
        main2 = main_event(p1[-1])
        if main1 and main2 and main1 != main2:
            return None
        p1 = p1[:-1] + (tuple(sorted(set(p1[-1] + tuple(a for a in p2[0] if a != event_type)))),)
        p2 = p2[1:]
    return p1 + p2

def simplify_binding(binding):
    res = ()
    for part in binding:
        res = path_join(res, (part,))
        if res is None:
            return res
    for idx in range(1, len(res)):
        prev = res[idx-1]
        part = res[idx]
        if "KeyRelease" in part and "KeyPress" in prev:
            res = res[:idx] + (("KeyRelease",),) + res[idx+1:]
        elif "ButtonRelease" in part and "ButtonPress" in prev:
            res = res[:idx] + (("ButtonRelease",),) + res[idx+1:]
        elif "MotionNotify" in part and "ButtonPress" in prev:
            res = res[:idx] + (("MotionNotify",),) + res[idx+1:]
    return res

def filter_binding(binding):
    group = None
    for part in binding:
        for item in part:
            if item in EVENT_TYPES:
                if group is None:
                    for g in COMPATIBLE_EVENTS:
                        if item in g:
                            group = g
                elif item not in group:
                    return True
    return False


def format_binding(binding):
    def filter_item(item):
        if item.startswith("!"):
            return None
        elif item in EVENT_TYPES and item != "MotionNotify":
            return None
        elif item in NUMBERS:
            item = "Button" + item
        item = item.replace("Mask", "")
        item = item.replace("Notify", "")
        item = item.replace("XK_", "")
        return item
    return tuple(tuple(b for b in (filter_item(b) for b in a) if b) for a in binding)

def simplify_bindings(bindings):
    for (key, value) in bindings:
        if filter_binding(key): continue
        key = simplify_binding(key)
        if not key: continue
        key = format_binding(key)
        yield (key, value)

def invert(d):
    res = {}
    for name, value in d.items():
        if isinstance(value, tuple):
            if value[0] not in res:
                res[value[0]] = {value[1]:set([name])}
            elif value[1] not in res[value[0]]:
                res[value[0]][value[1]] = set([name])
            else:
                res[value[0]][value[1]].add(name)
        else:
            if value not in res:
                res[value] = set([name])
            else:
                res[value].add(name)
    return res

def format_input_config():
    global config
    
    configpath = os.path.expanduser(os.environ.get("GLASS_INPUT_CONFIG", "~/.config/glass/input.json"))
    with open(configpath) as f:
        config = yaml.load(f, Loader=yaml.SafeLoader)

    res = invert(dict(simplify_bindings(flatten(config["modes"]["base_mode"]))))

    def format_bindings(bindings, indent="    "):
        for binding in bindings:
            binding = ", ".join("+".join(a for a in b) for b in format_binding(binding))
            print("%s%s" % (indent, binding,))

    actions = sorted(res.keys())
    for name in actions:
        value = res[name]
        print(name)
        if isinstance(value, dict):
            for params, v in value.items():
                print("    %s" % (", ".join(params),))
                format_bindings(v, "        ")
        else:
            format_bindings(value)

