# About

This is a minimalistic compositing window manager for X with infinite
desktop, infinite zoom and infinite virtual window pixel resolution.

Compile with "make" and run with bin/wm.

# Key bindings

All key bindings require the Windows key (Super_L) to be pressed, in
addition to some other keys or mouse buttons.

Generally in these bindings

* Button1+Control counts as Button2
* Arrow keys count as moving the mouse while holding Button1
  * Arrow keys+Control count as moving the mouse while holding Button2
* PageUp / PageDown keys counts as scroll wheel (Button4, Button5)

Bold ones are implemented:

|Function&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;                                                                             |SuperL|CtrlL|Alt|Sift|Btn1|Btn2|Btn4|Btn5|Arrow|PgUp|PgDn|Home|Motion|
|-----------------------------------------------|------|-----|---|----|----|----|----|----|-----|----|----|----|------|
|**Pan & zoom to initial position**             |X     |     |   |    |    |    |    |    |     |    |    |X   |      |
|**Move window**                                |X     |     |   |    |X   |    |    |    |     |    |    |    |X     |
|**Move window**                                |X     |     |   |    |    |    |    |    |X    |    |    |    |      |
|**Pan screen**                                 |X     |     |   |    |    |X   |    |    |     |    |    |    |X     |
|**Pan screen**                                 |X     |X    |   |    |X   |    |    |    |     |    |    |    |X     |
|**Pan screen**                                 |X     |X    |   |    |    |    |    |    |X    |    |    |    |      |
|**Zoom screen in**                             |X     |     |   |    |    |    |X   |    |     |    |    |    |      |
|**Zoom screen in**                             |X     |     |   |    |    |    |    |    |     |X   |    |    |      |
|**Zoom screen out**                            |X     |     |   |    |    |    |    |X   |     |    |    |    |      |
|**Zoom screen out**                            |X     |     |X  |    |    |    |    |    |     |    |X   |    |      |
|**Zoom screen in to window**                   |X     |     |   |X   |    |    |X   |    |     |    |    |    |      |
|**Zoom screen in to window**                   |X     |     |   |X   |    |    |    |    |     |X   |    |    |      |
|Zoom screen out to next window                 |X     |     |   |X   |    |    |    |X   |     |    |    |    |      |
|Zoom screen out to next window                 |X     |     |   |X   |    |    |    |    |     |    |X   |    |      |
|**Decrease window resolution**                 |X     |     |X  |    |    |    |X   |    |     |    |    |    |      |
|**Decrease window resolution**                 |X     |     |X  |    |    |    |    |    |     |X   |    |    |      |
|**Increase window resolution**                 |X     |     |X  |    |    |    |    |X   |     |    |    |    |      |
|**Increase window resolution**                 |X     |     |X  |    |    |    |    |    |     |    |X   |    |      |
|**Set window resolution to 1:1 to screen**     |X     |     |X  |X   |    |    |X   |    |     |    |    |    |      |
|**Set window resolution to 1:1 to screen**     |X     |     |X  |X   |    |    |    |    |     |X   |    |    |      |
|**Set screen resolution to 1:1 to window**     |X     |     |X  |X   |    |    |    |X   |     |    |    |    |      |
|**Set screen resolution to 1:1 to window**     |X     |     |X  |X   |    |    |    |    |     |    |X   |    |      |
|**Zoom screen in to window at 1:1 resolution** |X     |     |   |X   |    |    |    |    |     |    |    |X   |      |
|Write shell command                            |X     |     |   |    |    |    |    |    |     |    |    |    |      |

# Protocols

## Window properties

* IG_COORDS float[4] - Coordinates for the window on the desktop. A client can change these to move and resize a window.
* IG_SIZE int[2] - horizontal and vertical resolution of the window in pixels. A client can change these to change the window resolution without automatically resizing the window. A ConfigureRequest however, changes both resolution and size (proportionally).
* IG_LAYER atom - the desktop layer to place this window in
* DISPLAYSVG string - svg xml source code for an image to render instead of the window. Note: This rendering will support infinite zoom.

## ROOT properties

* IG_VIEWS atom[any] - a list of layers to display. Each layer needs to be further specified with the next two properties
* layer_LAYER atom - layer name to match on IG_LAYER on windows
* layer_VIEW float[4] - left,bottom,width,height of layer viewport (zoom and pan)
* IG_NOTIFY_MOTION window - Properties for describing the current mouse pointer position on the desktop
  * IG_ACTIVE_WINDOW window - the (top most) window under the mouse pointer (if any)
  * IG_NOTIFY_MOTION float[2*len(IG_VIEWS)] - mouse coordinates (x,y) for each desktop layer
* IG_ANIMATE window - event destination for animation events

## Animations

Window properties:

* prop_ANIMATE any[any] animate the value of the property pop to eventually be set to the value of this property. The animation is started by sending an IG_ANIMATE message.

The IG_ANIMATE message is a ClientMessage with the following properties:

    window = root.IG_ANIMATE
    type = "IG_ANIMATE"
    data[0] window = window to animate on
    data[1] atom = prop
    data[2] float = animation time in seconds


# Resources

* https://jichu4n.com/posts/how-x-window-managers-work-and-how-to-write-one-part-i/
* http://www.talisman.org/~erlkonig/misc/x11-composite-tutorial/

## Reference documentation
* Graphics
  * [Xlib](https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html)
  * [glX](https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/#glX)
* Window manager protocols
  * [ICCCM](https://www.x.org/releases/X11R7.6/doc/xorg-docs/specs/ICCCM/icccm.html)
  * [ewmh](https://www.freedesktop.org/wiki/Specifications/wm-spec/)

# License

GNU GPL v3, see the file LICENSE

The file xevent.c has a different, more permissible license, see the top of the file for details.
