# Based on https://github.com/jakesgordon/bin-packing/blob/master/js/packer.growing.js
# Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Jake Gordon and contributors
# Python port (c) 2020 Egil Moeller <redhog@redhog.org>

# This is a binary tree based bin packing algorithm that is more complex than
# the simple Packer (packer.js). Instead of starting off with a fixed width and
# height, it starts with the width and height of the first block passed and then
# grows as necessary to accomodate each subsequent block. As it grows it attempts
# to maintain a roughly square ratio by making 'smart' choices about whether to
# grow right or down.

# When growing, the algorithm can only grow to the right OR down. Therefore, if
# the new block is BOTH wider and taller than the current target then it will be
# rejected. This makes it very important to initialize with a sensible starting
# width and height. If you are providing sorted input (largest first) then this
# will not be an issue.

# A potential way to solve this limitation would be to allow growth in BOTH
# directions at once, but this requires maintaining a more complex tree
# with 3 children (down, right and center) and that complexity can be avoided
# by simply chosing a sensible starting block.

# Best results occur when the input blocks are sorted by height, or even better
# when sorted by max(width,height).

# Inputs:
# ------

#   blocks: array of any objects that have .w and .h attributes

# Outputs:
# -------

#   marks each block that fits with a .fit attribute pointing to a
#   node with .x and .y coordinates

# Example:
# -------

#   blocks = [
#     { w: 100, h: 100 },
#     { w: 100, h: 100 },
#     { w:  80, h:  80 },
#     { w:  80, h:  80 },
#     etc
#     etc
#   ]

#   var packer = GrowingPacker()
#   packer.fit(blocks)

#   for n in range(len(blocks)):
#     block = blocks[n]
#     if block.fit:
#       Draw(block.fit.x, block.fit.y, block.w, block.h)

class GrowingPacker(object):
  def fit(self, blocks, view_width, view_height, sorting="area"):
    aspect_ratio = view_width / view_height
    if sorting == "area":
      key = lambda b: b["w"] * b["h"]
    elif sorting == "circumference":
      key = lambda b: b["w"] + b["h"]
    elif sorting == "diagonal":
      key = lambda b: b["w"]**2 + b["h"]**2
    blocks.sort(key=key, reverse=True)
      
    self.aspect_ratio = aspect_ratio
    length = len(blocks)
    w = blocks[0]["w"] if length > 0 else 0
    h = blocks[0]["h"] if length > 0 else 0
    self.root = { "x": 0, "y": 0, "w": w, "h": h }
    for n in range(length):
      block = blocks[n]
      node = self.findNode(self.root, block["w"], block["h"])
      if node:
        block["fit"] = self.splitNode(node, block["w"], block["h"])
      else:
        block["fit"] = self.growNode(block["w"], block["h"])

  def findNode(self, root, w, h):
    if root.get("used"):
      return self.findNode(root["right"], w, h) or self.findNode(root["down"], w, h)
    elif (w <= root["w"]) and (h <= root["h"]):
      return root
    else:
      return None

  def splitNode(self, node, w, h):
    node["used"] = True
    node["down"]  = { "x": node["x"],     "y": node["y"] + h, "w": node["w"],     "h": node["h"] - h }
    node["right"] = { "x": node["x"] + w, "y": node["y"],     "w": node["w"] - w, "h": h          }
    return node

  def growNode(self, w, h):
    canGrowDown  = w <= self.root["w"]
    canGrowRight = h <= self.root["h"]

    # attempt to keep square-ish by growing right when height is much greater than width
    shouldGrowRight = canGrowRight and (self.root["h"] >= (self.root["w"] + w) / self.aspect_ratio)
    # attempt to keep square-ish by growing down  when width  is much greater than height
    shouldGrowDown  = canGrowDown  and (self.root["w"] >= (self.root["h"] + h) * self.aspect_ratio)

    if shouldGrowRight:
      return self.growRight(w, h)
    elif shouldGrowDown:
      return self.growDown(w, h)
    elif canGrowRight:
     return self.growRight(w, h)
    elif canGrowDown:
      return self.growDown(w, h)
    else:
      return None # need to ensure sensible root starting size to avoid self happening

  def growRight(self, w, h):
    self.root = {
      "used": True,
      "x": 0,
      "y": 0,
      "w": self.root["w"] + w,
      "h": self.root["h"],
      "down": self.root,
      "right": { "x": self.root["w"], "y": 0, "w": w, "h": self.root["h"] }
    }
    node = self.findNode(self.root, w, h)
    if node:
      return self.splitNode(node, w, h)
    else:
      return None

  def growDown(self, w, h):
    self.root = {
      "used": True,
      "x": 0,
      "y": 0,
      "w": self.root["w"],
      "h": self.root["h"] + h,
      "down":  { "x": 0, "y": self.root["h"], "w": self.root["w"], "h": h },
      "right": self.root
    }
    node = self.findNode(self.root, w, h)
    if node:
      return self.splitNode(node, w, h)
    else:
      return None

