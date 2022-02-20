import inspect
import os
import sys

def get_calling_function(level=1):
    # This should be way faster than calling inspect.stack()...
    frame = inspect.currentframe()
    for i in range(level):
        frame = frame.f_back
    return "%s.%s" % (
        frame.f_globals["__name__"],
        frame.f_code.co_name)

def FUNCTIONALITY_ENABLED(base, entry=None, level=1, fn=None):
    if fn is None:
        fn = get_calling_function(level+1)
    elif not isinstance(fn, str):
        fn = "%s.%s" % (fn.__name__, fn.__module__)
    key = "%s.%s" % (base, fn)
    if entry is not None:
        key += "." + entry
    envkey = key.replace(".", "_")
    res = os.environ.get(envkey)
    if res == '1': return key
    if res == '0': return None
    while "_" in envkey:
        envkey, dummy = envkey.rsplit("_", 1)
        res = os.environ.get(envkey)
        if res == '1': return key
        if res == '0': return None
    return None

def DEBUG_ENABLED(entry, level=1):
    return FUNCTIONALITY_ENABLED("GLASS_DEBUG", entry, level+1)

def DEBUG(entry, s, level=1):
    key = DEBUG_ENABLED(entry, level+1)
    if not key: return
    sys.stderr.write("%s: %s" % (key, s))
    sys.stderr.flush()
