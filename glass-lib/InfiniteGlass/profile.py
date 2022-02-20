from . import debug
import cProfile

def profilable(filename):
    def profilable(fn):
        if not debug.FUNCTIONALITY_ENABLED("GLASS_PROFILE", fn=fn):
            return fn
        def wrapper(*arg, **kw):
            with cProfile.Profile() as pr:
                res = fn(*arg, **kw)
            pr.dump_stats(filename)
            return res
        return wrapper
    if isinstance(filename, str):
        return profilable
    fn = filename
    filename = "%s.%s.prof" % (fn.__module__, fn.__name__)
    return profilable(fn)
