import Xlib.display
import sys
import struct

def set_root_prop(args):
    args = []
    kws = {}
    for arg in args:
        if arg.startswith("--"):
            arg = arg[2:]
            value = True
            if "=" in arg:
                arg, value = arg.split("=")
            kws[arg] = value
        else:
            args.append(arg)

    display = Xlib.display.Display()
    root = display.screen(0).root

    for key, value in kws.items():
        key = display.intern_atom(key)
        items = value.split(" ")
        if value[0] in '0123456789-':
            if "." in value:
                itemtype = display.intern_atom("FLOAT")
                items = [float(item) for item in items]
                items = struct.pack("<" + "f" * len(items), *items)
            else:
                itemtype = display.intern_atom("INTEGER")
                items = [int(item) for item in items]
                items = struct.pack("<" + "i" * len(items), *items)
        elif value.startswith("@"):
            itemtype = display.intern_atom("STRING")
            res = []
            for item in items:
                with open(value[1:], "rb") as f:
                    res.append(f.read())
            items = ''.join(res)
        else:
            itemtype = display.intern_atom("ATOM")
            items = [display.intern_atom(item) for item in items]
            items = struct.pack("<" + "l" * len(items), *items)

        root.change_property(key, itemtype, 32, items)

    display.flush()

def widget(args):
    pass
    
set_root_prop(sys.argv[1:])
