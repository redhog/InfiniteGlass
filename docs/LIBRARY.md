# The InfiniteGlass python library

The InfiniteGlass python library wraps the
[python-xlib](https://github.com/python-xlib/python-xlib) library.

# Informational variables

`InfiniteGlass.keysyms` dictionary X keysyms and their values

`InfiniteGlass.symkeys` reverse dictionary of `keysyms` above.

`InfiniteGlass.eventmask.event_mask_map` dictionary from event name to
list of event mask names that cover said event.


# Main loop for event handling.

Event handlers can be registered on displays and on windows. See
`display.on()`, `window.on()` and `with display:` below.

# Extended poperty types

## FLOAT

Content is encoded as an 32bit IEEE float.

## JSON

JSON data is encode like STRING, but the content is guaranteed to be a
valid JSON string.

# Core object extensions

InfiniteGlass extends (monkey patches) many python-xlib objects with
short cut syntaxes for common problems, e.g. window property
manipulation.

## Display

`Xlib.display.Display` objects are extended with the followin methods:

`display.root` the root windows of screen 0 of the display.

`display.peek_event` like `display.next_event` but does not remove the
returned event from the event queue.

```
@display.on(event, mask)
def handler(display, event):
    pass
```

Register a andler for an event.

`with display:` at the exit of the block, process events and call
event handlers until there are no registered event handlers any more.

`display.keycode("KEYSYM_NAME")` look up a keycode for a display by a
keysym specified with its string name.

`display.mask_to_keysym` dictionary from mask name to list of keysyms
that set the mask on events when pressed. Example: `ShiftMask':
['XK_Shift_L', 'XK_Shift_R']`


## Windows

`Xlib.xobject.drawable.Window` objects are extended with the following
syntaxes:

`window.keys()` list of all property names (as strings).

`"MY_PROPERTY" in window` check if perty is set on window.

`window["MY_PROPERTY"]` or `window.get("MY_PROPERTY", default=None)`
retrieves a property value.

`window.items()` retrievs al properties, as a list of tuples of names
and values.

`window["MY_PROPERTY"] = value` sets a property to a value.

Window values will be returned as
`Xlib.xobject.drawable.Window` objects, ATOMs will be returned as
string values. STRING values are returned as byte sequences.

`del window["MY_PROPERTY"]` unsets a property.

Type conversions when getting and setting values:

  * array: depending on typecode
    * b, B: STRING
    * h, i, l: INTEGER
    * H, I, L: CARDINAL
    * f: FLOAT
  * `Xlib.xobject.drawable.Window`: WINDOW
  * int: INTEGER
  * float: FLOAT
  * bytes: STRING
  * dict: JSON
  * tuple (TYPE_NAME, value): TYPE_NAME
  * string:
    * If a keycode name: KEYCODE
    * If an X constant name: XCONST
    * If it starts with @: Read STRING value from file path
    * If URL:
      * file:///path: Read STRING value from file path
      * resource://package/path: Read STRING value from python package resource
      * data://value: STRING, value is encoded to UTF-8
      * eq://jsonpath: Evaluate a JSON-path on `{"window": window}` and set value to return value.
    * Otherwise: ATOM

```
@window.on_event(event=None, mask=None)
def handler(window, event):
   pass
```

Register an event handler for an event, with some event mask on the window.

```
@window.require("MY_PROPERTY")
def handler(self, value):
    pass
```

Call an event handler once when a property with a particular name is
first set on a window.

`window.send(destination_window, client_type, *values)` send a
`ClientMessage` to a window. `client_type` should be an atom name as a
string, values will be converted the same way as for window
properties.
