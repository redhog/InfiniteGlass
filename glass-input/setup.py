#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-input',
      version='0.1',
      description='Input handling (keyboard mapping) for InfiniteGlass',
      long_description='Input handling (keyboard mapping) for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "pyyaml",
          "rpdb",
          "numpy",
          "ortools",
          "rectangle-packer"
      ],
      entry_points={
          'console_scripts': [
              'glass-input = glass_input:main',
          ],
          'InfiniteGlass.modes': [
              "Mode = glass_input.mode:Mode",
              "GrabbedMode = glass_input.grabbed:GrabbedMode",
              "BaseMode = glass_input.base:BaseMode",
          ],
          'InfiniteGlass.actions': [
              "set_focus = glass_input.actions.focus:set_focus",
              "focus_follows_mouse = glass_input.actions.focus:focus_follows_mouse",
              "focus_to_window_to_the_right = glass_input.actions.focus:focus_to_window_to_the_right",
              "focus_to_window_above = glass_input.actions.focus:focus_to_window_above",
              "focus_to_window_to_the_left = glass_input.actions.focus:focus_to_window_to_the_left",
              "focus_to_window_below = glass_input.actions.focus:focus_to_window_below",

              "pan = glass_input.actions.pan:pan",
              "zoom_to_window_to_the_right = glass_input.actions.pan:zoom_to_window_to_the_right",
              "zoom_to_window_above = glass_input.actions.pan:zoom_to_window_above",
              "zoom_to_window_to_the_left = glass_input.actions.pan:zoom_to_window_to_the_left",
              "zoom_to_window_below = glass_input.actions.pan:zoom_to_window_below",
              
              "zoom = glass_input.actions.zoom:zoom",
              "zoom_in = glass_input.actions.zoom:zoom_in",
              "zoom_out = glass_input.actions.zoom:zoom_out",

              "zoom_to_window = glass_input.actions.zoom_to:zoom_to_window",
              "zoom_to_fewer_windows = glass_input.actions.zoom_to:zoom_to_fewer_windows",
              "zoom_to_more_windows = glass_input.actions.zoom_to:zoom_to_more_windows",
              "zoom_home = glass_input.actions.zoom_to:zoom_home",

              "item_pan = glass_input.actions.item_pan:item_pan",
              "item_pan_mouse = glass_input.actions.item_pan:item_pan_mouse",
              
              "item_zoom_in = glass_input.actions.item_zoom:item_zoom_in",
              "item_zoom_out = glass_input.actions.item_zoom:item_zoom_out",

              "item_resize = glass_input.actions.item_resize:item_resize",
              "item_resize_mouse = glass_input.actions.item_resize:item_resize_mouse",

              "item_zoom_1_1_to_sreen = glass_input.actions.item_zoom_to:item_zoom_1_1_to_sreen",
              "item_zoom_in_or_1_1 = glass_input.actions.item_zoom_to:item_zoom_in_or_1_1",
              "item_zoom_out_or_1_1 = glass_input.actions.item_zoom_to:item_zoom_out_or_1_1",
              "item_zoom_1_1_to_window = glass_input.actions.item_zoom_to:item_zoom_1_1_to_window",
              "zoom_1_1_1 = glass_input.actions.item_zoom_to:zoom_1_1_1",

              "keymap = glass_input.actions.actions:keymap",
              "shell = glass_input.actions.actions:shell",
              "timer = glass_input.actions.actions:timer",
              "counter = glass_input.actions.actions:counter",
              "inc = glass_input.actions.actions:inc",
              "pop = glass_input.actions.actions:pop",
              "toggle_ghosts_enabled = glass_input.actions.actions:toggle_ghosts_enabled",
              "toggle_overlay = glass_input.actions.actions:toggle_overlay",
              "send_exit = glass_input.actions.actions:send_exit",
              "send_debug = glass_input.actions.actions:send_debug",
              "send_close = glass_input.actions.actions:send_close",
              "send_sleep = glass_input.actions.actions:send_sleep",
              "toggle_sleep = glass_input.actions.actions:toggle_sleep",
              "reload = glass_input.actions.actions:reload",
              "send_island_create = glass_input.actions.actions:send_island_create",

              "tile_visible = glass_input.actions.tile:tile_visible",
              "tile_visible_to_1_1 = glass_input.actions.tile:tile_visible_to_1_1",
              
              "ifnowin = glass_input.actions.modeswitch:ifnowin",

              "island_toggle_sleep = glass_input.actions.islands:island_toggle_sleep",
              "island_delete = glass_input.actions.islands:island_delete",
              "island_ungroup = glass_input.actions.islands:island_ungroup",
          ]
      },
      package_data={'glass_input': ['*.json']},
      include_package_data=True
  )
