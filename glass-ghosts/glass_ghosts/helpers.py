def expand_property(window, name):
    res = {name: window[name]}
    if name == "WM_CLIENT_LEADER":
        try:
            res["SM_CLIENT_ID"] = res["WM_CLIENT_LEADER"]["SM_CLIENT_ID"]
        except:
            pass
    return res

def tuplify(value):
    if isinstance(value, list):
        return tuple(value)
    return value
