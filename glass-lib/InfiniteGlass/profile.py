from . import debug
import cProfile

def profilable(filename):
    def profilable(fn):
        if not debug.FUNCTIONALITY_ENABLED("GLASS_PROFILE", fn=fn):
            return fn
        def wrapper(*arg, **kw):
            print("AAAAAAAAAAAA1", fn.__module__, fn.__name__, arg, kw)
            with cProfile.Profile() as pr:
                print("AAAAAAAAAAAA2", fn.__module__, fn.__name__, arg, kw)
                res = fn(*arg, **kw)
                print("AAAAAAAAAAAA3", fn.__module__, fn.__name__, arg, kw)
            if not isinstance(filename, str):
                filename = "%s.%s.prof" % (filename.__module__, filename.__name__)
            pr.dump_stats(filename)
            return res
        return wrapper
    if isinstance(filename, str):
        return profilable
    print("XXXXXXXXXXXX", filename.__module__, filename.__name__)
    return profilable(filename)
