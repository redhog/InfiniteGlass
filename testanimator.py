import InfiniteGlass; d = InfiniteGlass.Display()
import time
import sys

name = sys.argv[1]
l = float(sys.argv[2])
h = float(sys.argv[3])
t = float(sys.argv[4])

anim = d.root["IG_ANIMATE"]
d.root[name] = l
d.root[name + "_ANIMATE"] = h
anim.send(anim, "IG_ANIMATE", d.root, name, t)
while True:
    time.sleep(1)
    print(d.root[name])
