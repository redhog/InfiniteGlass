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
* Arrow keys + Alt counts as scroll wheel (Button4, Button5)

Bold ones are implemented:

|Function&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;                                            |SuperL|CtrlL|Alt|Sift|Btn1|Btn2|Btn4|Btn5|Up|Down|Left|Right|Home|Motion|
|-----------------------------------------------|------|-----|---|----|----|----|----|----|--|----|----|-----|----|------|
|**Pan to initial position**                    |X     |     |   |    |    |    |    |    |  |    |    |     |X   |      |
|**Move window**                                |X     |     |   |    |X   |    |    |    |  |    |    |     |    |X     |
|**Move window**                                |X     |     |   |    |    |    |    |    |X |    |    |     |    |      |
|**Move window**                                |X     |     |   |    |    |    |    |    |  |X   |    |     |    |      |
|**Move window**                                |X     |     |   |    |    |    |    |    |  |    |X   |     |    |      |
|**Move window**                                |X     |     |   |    |    |    |    |    |  |    |    |X    |    |      |
|**Pan screen**                                 |X     |     |   |    |    |X   |    |    |  |    |    |     |    |X     |
|**Pan screen**                                 |X     |X    |   |    |X   |    |    |    |  |    |    |     |    |X     |
|**Pan screen**                                 |X     |X    |   |    |    |    |    |    |X |    |    |     |    |      |
|**Pan screen**                                 |X     |X    |   |    |    |    |    |    |  |X   |    |     |    |      |
|**Pan screen**                                 |X     |X    |   |    |    |    |    |    |  |    |X   |     |    |      |
|**Pan screen**                                 |X     |X    |   |    |    |    |    |    |  |    |    |X    |    |      |
|**Zoom screen in**                             |X     |     |   |    |    |    |X   |    |  |    |    |     |    |      |
|**Zoom screen in**                             |X     |     |X  |    |    |    |    |    |X |    |    |     |    |      |
|**Zoom screen out**                            |X     |     |   |    |    |    |    |X   |  |    |    |     |    |      |
|**Zoom screen out**                            |X     |     |X  |    |    |    |    |    |  |X   |    |     |    |      |
|Zoom screen in to window                       |X     |     |   |X   |    |    |X   |    |  |    |    |     |    |      |
|Zoom screen in to window                       |X     |     |X  |X   |    |    |    |    |X |    |    |     |    |      |
|Zoom screen out to next window                 |X     |     |   |X   |    |    |    |X   |  |    |    |     |    |      |
|Zoom screen out to next window                 |X     |     |X  |X   |    |    |    |    |  |X   |    |     |    |      |
|Decrease window resolution                     |X     |     |X  |    |    |    |X   |    |  |    |    |     |    |      |
|Increase window resolution                     |X     |     |X  |    |    |    |    |X   |  |    |    |     |    |      |
|Set window resolution to 1:1 to screen         |X     |     |X  |X   |    |    |X   |    |  |    |    |     |    |      |
|Set screen resolution to 1:1 to window         |X     |     |X  |X   |    |    |    |X   |  |    |    |     |    |      |
|Write shell command                            |X     |     |   |    |    |    |    |    |  |    |    |     |    |      |

  
# Resources

* https://jichu4n.com/posts/how-x-window-managers-work-and-how-to-write-one-part-i/
* http://www.talisman.org/~erlkonig/misc/x11-composite-tutorial/

## Reference documentation
* https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html
* https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/#glX
