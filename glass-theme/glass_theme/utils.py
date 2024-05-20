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

@contextlib.contextmanager
def open_file(name):
    name = os.path.expanduser(name)
    if name.startswith("resource://"):
        pkg, name = name.split("://")[1].split("/", 1)
        with pkg_resources.resource_stream(pkg, name) as f:
            yield f
    else:
        with open(name) as f:
            yield f

def read_file(name):
    with open_file(name) as f:
        return f.read()
