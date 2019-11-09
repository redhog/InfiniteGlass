The components communicate between each other using window properties
and ClientMessages. These protocols can also be utilized by other
clients to streamline their interaction with the window manager and to
provide a better experience to the user.

When implementing new protocols, properties extending the data model
of client windows are to be preffered over ClientMessages implementing
actions.

# Renderer

Window rendering is implemented by the compositor, glass-renderer. It
is controlled by properties both on individual client windows and on
the root window.

## Shader programs

Rendering is controlled by OpenGL shader programs. The programs must be specified on the root window, and can then be selected individually for each window. All window properties are available to the shader code as uniforms. Note: You must specify the right type as well as item count, e.g. "vec4 MY_PROP" corresponds to MY_PROP being of type XA_FLOAT and having 4 items.

Shader programs are responsible for drawing in two modes - normal visible mode, and a mode used for picking, where each pixel value corresponds to the window id and coordinates of that window.

## Window properties

* IG_SHADER atom - the shader to use to render this window.
* IG_COORDS float[4] - Coordinates for the window on the desktop. A
  client can change these to move and resize a window.
* IG_SIZE int[2] - horizontal and vertical resolution of the window in
  pixels. A client can change these to change the window resolution
  without automatically resizing the window. A ConfigureRequest
  however, changes both resolution and size (proportionally).
* IG_LAYER atom - the desktop layer to place this window in
* DISPLAYSVG string - svg xml source code for an image to render
  instead of the window. Note: This rendering will support infinite
  zoom.

## ROOT properties

* IG_SHADERS atom[any] - a list of shader programs. Each program needs
  yo be further specified with the next three properties
  * shader_GEOMETRY string - geometry shader source code
  * shader_VERTEX string vertex shader source code
  * shader_FRAGMENT string fragment shader source code
* IG_VIEWS atom[any] - a list of layers to display. Each layer needs
  to be further specified with the next two properties
  * layer_LAYER atom - layer name to match on IG_LAYER on windows
  * layer_VIEW float[4] - left,bottom,width,height of layer viewport
  (zoom and pan)
* IG_ANIMATE window - event destination for animation events

The user should make sure to provide a shader called IG_SHADER_DEFAULT.

The user should make sure to provide a view called IG_VIEW_MENU
matching the window LAYER IG_LAYER_MENU, with a viewport of 0,0,1,0.75
(or whatever aspect ration your screen is). This layer will be used to
display override redirect windows, such as popup-menus.

Example root properties:

    IG_VIEWS=[IG_VIEW_DESKTOP, IG_VIEW_OVERLAY, IG_VIEW_MENU]

    IG_VIEW_MENU_VIEW=[0.0, 0.0, 1.0, 0.75]
    IG_VIEW_OVERLAY_VIEW=[0.0, 0.0, 1.0, 0.75]
    IG_VIEW_DESKTOP_VIEW=[0.0, 0.0, 1.0, 0.75]

    IG_VIEW_MENU_LAYER=IG_LAYER_MENU
    IG_VIEW_OVERLAY_LAYER=IG_LAYER_OVERLAY
    IG_VIEW_DESKTOP_LAYER=IG_LAYER_DESKTOP

# Animator

Animations are implemented by glass-animator and controlled by setting
window properties and sending ClientMessages.

To animate the value of the property 'prop' from its current value to
a new value, the property prop_ANIMATE should be set to the new value
on the same window. The animation is then started by sending a
ClientMessage with the following properties:

    window = The window pointed to by the IG_ANIMATE property of the
             root window
    type = "IG_ANIMATE"
    data[0] window = window with the property to animate
    data[1] atom = the property to animate, 'prop'
    data[2] float = animation time in seconds

# Coordinate systems

Windows have coordinates in the desktop (space) coordinate system, which is similar to the OpenGL one - x grows towards right, y upwards. Desktop coordinates have no natural units, as the desktop can be zoomed to any level, and there is nothing special about the 0 zoom level.

Window coordinates are for the top left corner of windows. Width and height are in the same units as x and y. Therefore, the coordinates occupied by a window ranges from ]x..x+w[,]y-h..y[.

# About the FLOAT datatype

The FLOAT datatype is encoded as a 32 bit float stored in a 32 bit item in properties and events according to the normal 32 bit item rules of XGetWindowProperty etc - that is, it is stored on the X server in network byte order, and converted to/from local byte order by Xlib. Note: On 64 bit platforms, XGetWindowProperty returns an array of long, which are 64, not 32 bits each. That means that the whole array CAN NOT be casted to an array of float, but each array item must be reinterpreted separately:

    float items[nr_items];
    XGetWindowProperty(display, window, property_name_atom, 0, sizeof(float)*nr_items, 0, AnyPropertyType,
                       &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
    if (type_return != Success) return NULL;
    for (int i = 0; i < nr_items; i++) {
     items[i] = *(float *) (i + (long *) prop_return);
    }
