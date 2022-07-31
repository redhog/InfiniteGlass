# Based on https://raw.githubusercontent.com/IanTrudel/ImageCloud/master/cloud_packer.py
# Copyright (c) 2022 Ian Trudel

# Window Cloud Packer
# Packing a list of tuples (id, w, h)
# Returning a list of tuples (id, x, y)
import math

class CloudPacker():
   # TODO: assign a weight to each window according to their area, width, height.
   #       These will be used to determine the order in which the windows are placed,
   #       in accordance with the aspect ratio of the root window.
   def organic_sort(self, block):
      return (block["w"] * block["h"])

   def fit(self, blocks, view_width, view_height, margin=0, sorting="organic"):
      self.view_width = view_width
      self.view_height = view_height
      self.aspect_ratio = view_width / view_height
      self.margin = margin # TODO: implement margin

      blocks.sort(key=self.organic_sort, reverse=True)

      placements = []
      spiral = self.spiral_generator(20, 1.5)

      for position, block in enumerate(blocks):
         window = block["window"]
         w = block["w"]
         h = block["h"]
         px, py = next(spiral)
         dx, dy = self.find_window_center_coordinates([window, px, py, w, h])

         block["fit"] = {"x": dx, "y": dy}

         while(position and self.is_window_intersect_view(block, placements)):
            px, py = next(spiral)
            dx, dy = self.find_window_center_coordinates([window, px, py, w, h])
            block["fit"] = {"x": dx, "y": dy}

         placements.append(block)

      return placements

   def spiral_generator(self, step=10, radius=1.0):
      h = (self.view_width / 2)
      k = (self.view_height / 2)
      theta = 0
      r = 0

      while True:
         x = h + (r * self.aspect_ratio) * math.cos(theta)
         y = k + (r * 1 / self.aspect_ratio) * math.sin(theta)

         yield (round(x), round(y))

         theta += step
         r += radius

         if (theta > 360):
            theta = 0

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
