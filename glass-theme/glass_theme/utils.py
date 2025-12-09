import InfiniteGlass.utils
import importlib
import pkg_resources
import os.path
import contextlib

def import_function(name):
    module_name, cls_name = name.rsplit(".", 1)
    module = importlib.import_module(module_name)
    importlib.reload(module)
    return getattr(module, cls_name)
    
def instantiate_config(display, cls, args):
    if args is None: args = {}
    if len(args) == 1:
        clsname, args = next(iter(args.items()))
        if "." in clsname:
            cls = import_function(clsname)
    if args is None: args = {}    
    return cls(display, **args)

# FIXME: replaces usages of these
open_file = InfiniteGlass.utils.open_file
read_file = InfiniteGlass.utils.read_file
