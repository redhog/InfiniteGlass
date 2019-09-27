import importlib

keysymdef = importlib.__import__("Xlib.keysymdef", fromlist="*")

keysyms = {keyname: getattr(dictionary, keyname) 
           for dictionary in [getattr(keysymdef, name) for name in keysymdef.__all__]
           for keyname in dir(dictionary)
           if not keyname.startswith("_")}
