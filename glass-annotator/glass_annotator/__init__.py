from importlib.resources import files
import os

def substring_in_list(s, lst):
    for item in lst:
        if s in item:
            return True
    return False

def setup_annotator():
    preloads = []
    if "LD_PRELOAD" in os.environ:
        preloads = os.environ["LD_PRELOAD"].split(" ")
    if not substring_in_list('glass-annotator', preloads):
        preloads.append(str(files("glass_annotator") / "lib" / "glass-annotator"))
        os.environ["LD_PRELOAD"] = " ".join(preloads)
