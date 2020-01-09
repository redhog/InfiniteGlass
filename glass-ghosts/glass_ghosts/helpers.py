import slugify

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

def shadow_key(properties, use):
    def stringify(value):
        if value is None:
            return ""
        if isinstance(value, (list, tuple)):
            return "-".join(stringify(item) for item in value)
        if isinstance(value, bytes):
            try:
                return value.decode("utf-8")
            except:
                pass
        return str(value)
    return slugify.slugify(stringify([properties.get(name, None) for name in sorted(use)]))
