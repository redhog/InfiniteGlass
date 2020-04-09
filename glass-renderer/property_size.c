#include "property_size.h"
#include "property_int.h"
#include "item.h"
#include "shader.h"
#include "rendering.h"
#include "debug.h"
#include "glapi.h"

void property_size_init(PropertyTypeHandler *prop) { prop->type = XA_INTEGER; prop->name = ATOM("IG_SIZE"); }
void property_size_load(Property *prop) {
  if (prop->window == root) return;
  if (prop->type == None) return;
 
  Item *item = (Item *) item_get_from_window(prop->window, False);
  if (!item) return;
  
  XWindowChanges values;
  values.width = prop->values.dwords[0];
  values.height = prop->values.dwords[1];
  XWindowAttributes attr;
  XGetWindowAttributes(display, prop->window, &attr);

  if (attr.width != values.width || attr.height != values.height) {
    // Do not allow way to big windows, as that screws up OpenGL and X11 and everything will crash...
    if (values.width < 0 || values.height < 0 || values.width > overlay_attr.width * 5 || values.height > overlay_attr.height * 5) {
      long arr[2];
      arr[0] = attr.width;
      arr[1] = attr.height;
      DEBUG("event.size", "%ld: Warning IG_SIZE outside of bounds, resetting to %i,%i\n", prop->window, attr.width, attr.height);
      XChangeProperty(display, prop->window, ATOM("IG_SIZE"), XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
    } else {
      DEBUG("event.size", "%ld: SIZE CHANGED TO %i,%i\n", prop->window, values.width, values.height);
      XConfigureWindow(display, prop->window, CWWidth | CWHeight, &values);
      item_update((Item *) item);
    }
  }
}

PropertyTypeHandler property_size = {
  &property_size_init, &property_size_load,
  &property_int_free, &property_int_to_gl, &property_int_print, &property_int_load_program, &property_int_free_program};
