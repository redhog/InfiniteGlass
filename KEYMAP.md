# Key bindings

Key bindings are implemented by input.py

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
