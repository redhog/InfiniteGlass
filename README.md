# About

This is a minimalistic compositing window manager for X with infinite
desktop, infinite zoom and infinite virtual window pixel resolution.

Features:

* Windows are glued to an infinite desktop to some coordinates and size
* Window pixel resolution is independent of their size on the desktop
* The user can zoom / pan to any view of the desktop
* There are shortcut commands to zoom to a window, and to make a window have a 1:1 resolution to the screen
* Window placement is stored between sessions
  * Windows that are closed leave a ghost window in their place that can be moved, resized and deleted.
  * When a window with the same name reappears, it takes the place of the ghost window if it still exists.
  * Ghost windows are stored in a file between sessions.
* Window content / application state is stored between sessions for supported clients
  * Applications supporting the XSM protocol can be closed and saved and later restored, in the same session or another.
* Infinitely zoomable SVG for buttons etc
  * A window can display an SVG image instead of content, and the SVG will render to the current screen resolution, no matter the zoom level.

Compile and run with "make"

* [Key bindings](KEYMAP.md)
* [Protocols](PROTOCOLS.md)
* [References](REFERENCES.md)

# License

GNU GPL v3, see the file LICENSE

The file xevent.c has a different, more permissible license, see the top of the file for details.
