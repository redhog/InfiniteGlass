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

def DEBUG_ENABLED(entry, level=1):
    key = "%s.%s.%s" % ("GLASS_DEBUG", get_calling_function(level+1), entry)
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

def DEBUG(entry, s, level=1):
    key = DEBUG_ENABLED(entry, level+1)
    if not key: return
    sys.stderr.write("%s: %s" % (key, s))
    sys.stderr.flush()
