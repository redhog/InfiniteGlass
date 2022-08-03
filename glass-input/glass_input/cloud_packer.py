# Based on https://raw.githubusercontent.com/IanTrudel/ImageCloud/master/cloud_packer.py
# Copyright (c) 2022 Ian Trudel

# Window Cloud Packer
# Packing a list of tuples (id, w, h)
# Returning a list of tuples (id, x, y)
import math

class DualBinarySearch(object):
    """Generates a sequence that first grows exponentially, and then
    grows or shrinks with logarithmically smaller and smaller steps."""
    def __init__(self, start=0, step=1, factor=2, grow=True, grow_only=True):
        self.pos = start
        self.step = step
        self.factor = factor
        self.grow_only = grow_only
        self.grow = grow
    def __iter__(self):
        return self
    def direction(self, grow = True):
        if grow == self.grow:
            return
        if self.grow_only:
            self.step /= self.factor            
        self.grow_only = False
        self.grow = grow
    def __next__(self):
        
        if self.grow:
            self.pos += self.step
        else:
            self.pos -= self.step
        if self.grow_only:
            self.step *= self.factor
        else:
            self.step /= self.factor
        return self.pos

class CloudPacker():
   # TODO: assign a weight to each window according to their area, width, height.
   #       These will be used to determine the order in which the windows are placed,
   #       in accordance with the aspect ratio of the root window.
   def __init__(self, step=1, radius=0.001):
      self.step = float(step)
      self.radius = float(radius)
      
   def organic_sort(self, block):
      return (block["w"] * block["h"])

   def fit(self, blocks, view_width, view_height, margin=0, sorting="organic"):
      self.aspect_ratio = view_width / view_height
      
      self.iss = []
      
      blocks.sort(key=self.organic_sort, reverse=True)

      mind = min([math.sqrt(shape["w"]**2+shape["h"]**2) for shape in blocks])
      self.adjusted_radius = self.radius * mind
      
      self.placements = []
      for position, block in enumerate(blocks):
          self.find_fit(block, position)
          
      return self.placements

   def find_fit(self, block, position):
      s = DualBinarySearch(step=position)
      si = iter(s)
      for i in si:
         self.iss.append(i)
         self.set_fit(i, block)
         overlapping = self.is_window_intersect_view(block, self.placements)
         if not overlapping:
            break
      s.direction(overlapping)
      best = block["fit"]
      for c, i in enumerate(si):
         self.iss.append(i)
         if c > 20: break
         self.set_fit(i, block)
         overlapping = self.is_window_intersect_view(block, self.placements)
         if not overlapping:
            best = block["fit"]
         s.direction(overlapping)
      block["fit"] = best

      self.adjust_fit(block, "x")
      self.adjust_fit(block, "y")
      
      self.placements.append(block)
       
   def adjust_fit(self, block, coord):
      best = block["fit"][coord]
      positive = block["fit"][coord] > 0
      s = DualBinarySearch(start=block["fit"][coord], step=abs(block["fit"][coord] / 2), grow_only=False, grow=not positive)
      si = iter(s)
      for c, i in enumerate(si):
         if c > 40: break
         block["fit"][coord] = i
         overlapping = self.is_window_intersect_view(block, self.placements)
         if overlapping:
            break
      s.direction(positive)
      for c, i in enumerate(si):
         if c > 40: break
         block["fit"][coord] = i
         overlapping = self.is_window_intersect_view(block, self.placements)
         #print(block["window"], "B", i, overlapping)
         if not overlapping:
            best = i
         s.direction(overlapping if positive else not overlapping)
      block["fit"][coord] = best
      
   def set_fit(self, i, block):
      window = block["window"]
      w = block["w"]
      h = block["h"]
      px, py = self.get_position(i)
      dx, dy = self.find_window_center_coordinates([window, px, py, w, h])
      block["fit"] = {"x": dx, "y": dy}
      
   def get_position(self, i):
      thetafull = (self.step * i)
      theta = thetafull % 360
      r = self.adjusted_radius * math.floor(thetafull / 360)
      
      x = (r * self.aspect_ratio) * math.cos(theta)
      y = (r * 1 / self.aspect_ratio) * math.sin(theta)
      
      return (round(x), round(y))

   def is_window_intersect_view(self, window, windows):
      for w in windows:
         if (window["window"] != w["window"] and self.is_window_intersect(window, w)):
            return True
      return False

   def is_window_intersect(self, b1, b2):
      return (    b1["fit"]["x"] < b2["fit"]["x"] + b2["w"]
              and b1["fit"]["x"] + b1["w"] > b2["fit"]["x"]
              and b1["fit"]["y"] < b2["fit"]["y"] + b2["h"]
              and b1["fit"]["y"] + b1["h"] > b2["fit"]["y"])

   def find_window_center_coordinates(self, window):
      _, x, y, w, h = window
      return (x - (w / 2), y - (h / 2))
