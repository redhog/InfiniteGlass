import os
import sys
from importlib.resources import files

def main():
    bin_path = str(files("glass_renderer") / "bin" / "glass-renderer")
    os.execv(bin_path, [bin_path] + sys.argv[1:])
