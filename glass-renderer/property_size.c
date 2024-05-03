#include "property_size.h"
#include "property_int.h"
#include "item.h"
#include "shader.h"
#include "rendering.h"
#include "mainloop.h"
#include "debug.h"
#include "glapi.h"

void property_size_init(PropertyTypeHandler *prop) { prop->type = XA_INTEGER; prop->name = ATOM("IG_SIZE"); }

void property_size_load_geom(Property *prop, xcb_get_geometry_reply_t *reply, xcb_generic_error_t *error) {
 if (!reply) return;
 
  int width = prop->values.dwords[0];
  int height = prop->values.dwords[1];

  Item *item = (Item *) item_get_from_window(prop->window, False);
  if (!item) return;

  if (reply->width != width || reply->height != height) {
    // Do not allow way to big windows, as that screws up OpenGL and X11 and everything will crash...
    if (width < 0 || height < 0 || width > overlay_attr.width * 5 || height > overlay_attr.height * 5) {
      int arr[2];
      arr[0] = reply->width;
      arr[1] = reply->height;
      DEBUG("event.size", "%ld: Warning IG_SIZE outside of bounds, resetting to %i,%i\n", prop->window, reply->width, reply->height);
      xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, prop->window, ATOM("IG_SIZE"), XA_INTEGER, 32, 2, (void *) arr);    
    } else {
      DEBUG("event.size", "%ld: SIZE CHANGED TO %i,%i\n", prop->window, width, height);
      const uint32_t values[] = {width, height};
      xcb_configure_window(xcb_display, prop->window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
      item_update((Item *) item);
    }
  }
}


void property_size_load(Property *prop) {
  if (prop->window == root) return;
  if (prop->type == None) return;
 
  xcb_get_geometry_cookie_t cookie = xcb_get_geometry(xcb_display, prop->window);
  MAINLOOP_XCB_DEFER(cookie, &property_size_load_geom, (void *) prop);  
}

PropertyTypeHandler property_size = {
  .init=&property_size_init,
  .load=&property_size_load,
  .to_gl=&property_int_to_gl,
  .print=&property_int_print,
  .load_program=&property_int_load_program,
  .free_program=&property_int_free_program
};
