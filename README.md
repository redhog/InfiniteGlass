# About

This is a minimalistic compositing window manager for X with infinite
desktop, infinite zoom and infinite virtual window pixel resolution.

Compile with "make" and run with bin/wm.

# Key bindings

All key bindings require the Windows key (Super_L) to be pressed, in
addition to some other keys or mouse buttons.

* Super_L + Button1 + move
  Move window

* Super_L + Button2 + move
  Pan the screen

* Super_L + Button4 (Scroll wheel up)
  Zoom screen in

* Super_L + Button5 (Scroll wheel down)
  Zoom screen out

* Super_L + Button4 (Scroll wheel up) + Shift
  Zoom screen in to the window under the mouse pointer

* *Super_L + Button5 (Scroll wheel down) + Shift*
  Zoom screen out to the window under the mouse cursor, or to include
  one more window

* Super_L + Control_L + Up
  Zoom screen in

* Super_L + Control_L + Down
  Zoom screen out



# Future key bindings (ideas)

* Super_L + Up/Down/Right/Left
  Pan screen up/down/right/left

* Super_L + Up + Shift
  Zoom screen in to the window under the mouse pointer

* Super_L + Down + Shift
  Zoom screen out to the window under the mouse cursor, or to include
  one more window

* Super_L + Button4 +  (Scroll wheel up) + Alt_L
  Zoom window in, e.g. decrease it's size in pixels

* Super_L + Button5 (Scroll wheel down) + Alt_L
  Zoom window out, e.g. increase it's size in pixels
  
* Super_L + Button4 +  (Scroll wheel up) + Alt_L + Shift
* Super_L + Button5 (Scroll wheel down) + Alt_L + Shift
  Zoom window to 1:1, that is, so that each pixel in the window
  corresponds to 1 pixel on the screen.

* Super_L + any character key
  Start writing a shell command. Mode is exited by ENTER or by
  pressing and releasing Super_L.
  
# Resources

* https://jichu4n.com/posts/how-x-window-managers-work-and-how-to-write-one-part-i/
* http://www.talisman.org/~erlkonig/misc/x11-composite-tutorial/

## Reference documentation
* https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html
* https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/#glX
