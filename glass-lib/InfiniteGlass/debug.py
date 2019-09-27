import inspect
import os
import sys

def DEBUG_ENABLED(entry, level=1):
    key = ".".join(item for item in ("GLASS_DEBUG", inspect.stack()[level].frame.f_globals["__name__"], inspect.stack()[level].function, entry)
                   if item != "<module>")    
    key = key.replace(".", "_")
    res = os.environ.get(key)
    if res == '1': return 1
    if res == '0': return 0
    while "_" in key:
        key, dummy = key.rsplit("_", 1)
        res = os.environ.get(key)
        if res == '1': return 1
        if res == '0': return 0
    return 0

def DEBUG(entry, s, level=1):
    if not DEBUG_ENABLED(entry, 2): return
    key = ".".join(item for item in ("GLASS_DEBUG", inspect.stack()[level].frame.f_globals["__name__"], inspect.stack()[level].function, entry)
                   if item != "<module>")    
    sys.stderr.write("%s: %s" % (key, s))
    sys.stderr.flush()
