#include "glapi.h"
#include "xapi.h"
#include "item.h"
#include "view.h"
#include "shader.h"
#include "wm.h"
#include <limits.h>
#include "property.h"
#include "debug.h"
#include "rendering.h"
#include <X11/Xatom.h>

List *items_all = NULL;
Item *root_item = NULL;
 
Bool init_items() {
  return True;
}


void item_constructor(Item *item, Window window) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return = NULL;
  
  item->window = window;
  item->properties = NULL;
  item->prop_layer = NULL;
  item->prop_shader = NULL;
  item->prop_size = NULL;
  item->prop_coords = NULL;
  item->prop_draw_type = NULL;
  item->draw_cycles_left = 0;
  
  if (window == root) {
    Atom layer = ATOM("IG_LAYER_ROOT");
    XChangeProperty(display, window, ATOM("IG_LAYER"), XA_ATOM, 32, PropModeReplace, (void *) &layer, 1);
    Atom shader = ATOM("IG_SHADER_ROOT");
    XChangeProperty(display, window, ATOM("IG_SHADER"), XA_ATOM, 32, PropModeReplace, (void *) &shader, 1);
    item->is_mapped = True;
  } else {
    XGetWindowProperty(display, window, ATOM("IG_LAYER"), 0, sizeof(Atom), 0, XA_ATOM,
                       &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
    if (type_return == None) {
      Atom layer = ATOM("IG_LAYER_DESKTOP");
      XGetWindowAttributes(display, window, &item->attr);
      if (item->attr.override_redirect) {
        layer = ATOM("IG_LAYER_MENU");
      }
      XChangeProperty(display, window, ATOM("IG_LAYER"), XA_ATOM, 32, PropModeReplace, (void *) &layer, 1);
    }
    XFree(prop_return);

    long value = 1;
    XChangeProperty(display, window, ATOM("WM_STATE"), XA_INTEGER, 32, PropModeReplace, (void *) &value, 1);

    XGetWindowAttributes(display, window, &item->attr);
    item->is_mapped = item->attr.map_state == IsViewable; // FIXME: Remove is_mapped...

    XSelectInput(display, window, PropertyChangeMask);
    item->damage = XDamageCreate(display, item->window, XDamageReportNonEmpty);
  }

  XGetWindowProperty(display, window, ATOM("IG_DRAW_TYPE"), 0, sizeof(Atom), 0, XA_ATOM,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return == None) {
    Atom draw_type = ATOM("IG_DRAW_TYPE_POINTS");
    XChangeProperty(display, window, ATOM("IG_DRAW_TYPE"), XA_ATOM, 32, PropModeReplace, (void *) &draw_type, 1);
  }
  XFree(prop_return);
  
  item->properties = properties_load(window);
  item->prop_layer = properties_find(item->properties, ATOM("IG_LAYER"));
  item->prop_shader = properties_find(item->properties, ATOM("IG_SHADER"));
  item->prop_size = properties_find(item->properties, ATOM("IG_SIZE"));
  item->prop_coords = properties_find(item->properties, ATOM("IG_COORDS"));
  item->prop_draw_type = properties_find(item->properties, ATOM("IG_DRAW_TYPE"));
  
  if (window != root) {
    item_update_space_pos_from_window(item);
  }
  
  item->window_pixmap = 0;
  texture_initialize(&item->window_texture);
}
void item_destructor(Item *item) {
  texture_destroy(&item->window_texture);
}
void item_draw(Rendering *rendering) {
  if (rendering->item->is_mapped) {
    Item *item = (Item *) rendering->item;
    Shader *shader = rendering->shader;

    if (!rendering->view->picking) {
      if (item->draw_cycles_left > 0) {
        texture_from_pixmap(&item->window_texture, item->window_pixmap);
        item->draw_cycles_left--;
      }
     
      glUniform1i(shader->window_sampler_attr, rendering->texture_unit);
      glActiveTexture(GL_TEXTURE0 + rendering->texture_unit);
      glBindTexture(GL_TEXTURE_2D, item->window_texture.texture_id);
      glBindSampler(rendering->texture_unit, 0);
      rendering->texture_unit++;
    }

    
    properties_to_gl(root_item->properties, "root_", rendering);
    GL_CHECK_ERROR("item_draw_root_properties", "%ld.%s", item->window, rendering->shader->name_str);
    properties_to_gl(rendering->item->properties, "", rendering);
    GL_CHECK_ERROR("item_draw_properties", "%ld.%s", item->window, rendering->shader->name_str);
    
    glUniform1i(shader->picking_mode_attr, rendering->view->picking);
    glUniform4fv(shader->screen_attr, 1, rendering->view->screen);
    glUniform2i(shader->size_attr, rendering->view->width, rendering->view->height);
    glUniform1i(shader->border_width_attr, rendering->item->attr.border_width);

    DEBUG("setwin", "%ld\n", rendering->item->window);
    glUniform1i(shader->window_id_attr, rendering->item->window);
    
    GL_CHECK_ERROR("item_draw2", "%ld.%s", item->window, rendering->shader->name_str);
    
    DEBUG("draw_arrays", "%ld.draw(%ld items)\n", rendering->item->window, rendering->array_length);

    Atom prop_draw_type = ATOM("IG_DRAW_TYPE_POINTS");
    GLuint draw_type = GL_POINTS;
    if (item->prop_draw_type) {
      prop_draw_type = (Atom) item->prop_draw_type->values.dwords[0];
      if (prop_draw_type == ATOM("IG_DRAW_TYPE_POINTS")) draw_type = GL_POINTS;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_LINES")) draw_type = GL_LINES;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_LINE_STRIP")) draw_type = GL_LINE_STRIP;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_LINES_ADJACENCY")) draw_type = GL_LINES_ADJACENCY;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_LINE_STRIP_ADJACENCY")) draw_type = GL_LINE_STRIP_ADJACENCY;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_TRIANGLES")) draw_type = GL_TRIANGLES;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_TRIANGLE_STRIP")) draw_type = GL_TRIANGLE_STRIP;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_TRIANGLE_FAN")) draw_type = GL_TRIANGLE_FAN;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_TRIANGLES_ADJACENCY")) draw_type = GL_TRIANGLES_ADJACENCY;
      else if (prop_draw_type == ATOM("IG_DRAW_TYPE_TRIANGLE_STRIP_ADJACENCY")) draw_type = GL_TRIANGLE_STRIP_ADJACENCY;
    }  
    glDrawArrays(draw_type, 0, rendering->array_length);

    GL_CHECK_ERROR("item_draw3", "%ld.%s: %s(%ld)",
                   item->window, rendering->shader->name_str,
                   XGetAtomName(display, prop_draw_type), rendering->array_length);

    if (!rendering->view->picking) {
      XErrorEvent error;
      x_try();
      XDamageSubtract(display, item->damage, None, None);
      x_catch(&error);
    }
  }
}

void item_update(Item *item) {
  if (item->window == root) return;
  if (!item->is_mapped) return;
  item->_is_mapped = item->is_mapped;

  x_push_error_context("item_update_pixmap");
  
  // FIXME: free all other stuff if already created

  if (item->window_pixmap) {
    XFreePixmap(display, item->window_pixmap);
  }
  item->window_pixmap = XCompositeNameWindowPixmap(display, item->window);

  if (DEBUG_ENABLED("window.update")) {
    Window root_return;
    int x_return;
    int y_return;
    unsigned int width_return;
    unsigned int height_return;
    unsigned int border_width_return;
    unsigned int depth_return;

    int res = XGetGeometry(display, item->window_pixmap,
                           &root_return, &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return);
    DEBUG("window.update",
          "XGetGeometry(pixmap=%lu) = status=%d, root=%lu, x=%u, y=%u, w=%u, h=%u border=%u depth=%u\n",
           item->window_pixmap, res, root_return, x_return, y_return, width_return, height_return, border_width_return, depth_return);
  }
  
  texture_from_pixmap(&item->window_texture, item->window_pixmap);

  GL_CHECK_ERROR("item_update_pixmap2", "%ld", item->window);

  x_pop_error_context();  
}

Bool item_properties_update(Item *item, Atom name) {
  Bool res = properties_update(item->properties, name);
  if (res) {
    if (name == ATOM("IG_LAYER") && !item->prop_layer) item->prop_layer = properties_find(item->properties, ATOM("IG_LAYER"));
    if (name == ATOM("IG_SHADER") && !item->prop_shader) item->prop_shader = properties_find(item->properties, ATOM("IG_SHADER"));
    if (name == ATOM("IG_SIZE") && !item->prop_size) item->prop_size = properties_find(item->properties, ATOM("IG_SIZE"));
    if (name == ATOM("IG_COORDS") && !item->prop_coords) item->prop_coords = properties_find(item->properties, ATOM("IG_COORDS"));        
    if (name == ATOM("IG_DRAW_TYPE") && !item->prop_draw_type) item->prop_draw_type = properties_find(item->properties, ATOM("IG_DRAW_TYPE"));        
  }
  return res;
}

Shader *item_get_shader(Item *item) {
  Atom shader = ATOM("IG_SHADER_DEFAULT");
  if (item->prop_shader) shader = item->prop_shader->values.dwords[0];
  return shader_find(shaders, shader);
}
void item_print(Item *item) {
  float _coords[] = {-1.,-1.,-1.,-1.};
  float *coords = _coords;
  long width = -1, height = -1;
  if (item->prop_size) {
    width = item->prop_size->values.dwords[0];
    height = item->prop_size->values.dwords[1];
  }
  if (item->prop_coords) {
    coords = (float *) item->prop_coords->data;
  }
  printf("item(%ld):%s [%ld,%ld] @ %f,%f,%f,%f\n",
         item->window,
         item->is_mapped ? "" : " invisible",
         width,
         height,
         coords[0],
         coords[1],
         coords[2],
         coords[3]);
  printf("    window=%ld\n", item->window);
  properties_print(item->properties, stderr);
}

Item *item_create(Window window) {
  Item *item = (Item *) malloc(sizeof(Item));

  item->is_mapped = False;
  item->_is_mapped = False;
  item_constructor(item, window);
  item_add(item);
  item_update(item);
  return item;
}

void item_add(Item *item) {
  if (!items_all) items_all = list_create();
  list_append(items_all, (void *) item);
}

void item_remove(Item *item) {
  list_remove(items_all, (void *) item);
  item_destructor(item);
}

void item_update_space_pos_from_window(Item *item) {
  if (item->window == root) return;
  
  XGetWindowAttributes(display, item->window, &item->attr);

  item->x                   = item->attr.x;
  item->y                   = item->attr.y;
  int width                 = item->attr.width;
  int height                = item->attr.height;
  DEBUG("window.spacepos", "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, item->x, item->y, width, height);

  long arr[2] = {width, height};
  XChangeProperty(display, item->window, ATOM("IG_SIZE"), XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
  
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;
  XGetWindowProperty(display, item->window, ATOM("IG_COORDS"), 0, sizeof(float)*4, 0, AnyPropertyType,
                       &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    XFree(prop_return);
  } else {
    View *v = NULL;
    if (views && item->prop_layer && item->prop_layer->values.dwords) {
      v = view_find(views, (Atom) item->prop_layer->values.dwords[0]);
    }
    float coords[4];
    if (v) {
      coords[0] = v->screen[0] + (v->screen[2] * (float) item->x) / (float) v->width;
      coords[1] = v->screen[1] + v->screen[3] - (v->screen[3] * (float) item->y) / (float) v->height;
      coords[2] = (v->screen[2] * (float) width) / (float) v->width;
      coords[3] = (v->screen[3] * (float) height) / (float) v->height;
      DEBUG("position", "Setting item position from view for %ld (IG_COORDS missing).\n", item->window);
    } else {
      coords[0] = ((float) (item->x - overlay_attr.x)) / (float) overlay_attr.width;
      coords[1] = ((float) (overlay_attr.height - item->y - overlay_attr.y)) / (float) overlay_attr.width;
      coords[2] = ((float) (width)) / (float) overlay_attr.width;
      coords[3] = ((float) (height)) / (float) overlay_attr.width;
      DEBUG("position", "Setting item position to 0 for %ld (IG_COORDS & IG_LAYER missing).\n", item->window);
    }

    if (item->prop_layer && item->prop_layer->values.dwords && (Atom) item->prop_layer->values.dwords[0] == ATOM("IG_LAYER_MENU")) {
      DEBUG("menu.setup", "%ld: %d,%d[%d,%d]   %f,%f,%f,%f\n",
            item->window,
            item->attr.x, item->attr.y, item->attr.width, item->attr.height,
            coords[0],coords[1],coords[2],coords[3]);
   }
        
    long arr[4];
    for (int i = 0; i < 4; i++) {
      arr[i] = *(long *) &coords[i];
    }
    XChangeProperty(display, item->window, ATOM("IG_COORDS"), XA_FLOAT, 32, PropModeReplace, (void *) arr, 4);
  }
}

Item *item_get_from_window(Window window, int create) {
  if (items_all) {
    for (size_t idx = 0; idx < items_all->count; idx++) {
      Item *item = (Item *) items_all->entries[idx];
      if (item->window == window) return item;
    }
  }
  if (!create) return NULL;
  
  DEBUG("window.add", "Adding window %ld\n", window);
  return item_create(window);
}


void items_get_from_toplevel_windows() {
  XCompositeRedirectSubwindows(display, root, CompositeRedirectAutomatic);

  root_item = item_get_from_window(root, True);
  
  XGrabServer(display);
  
  Window returned_root, returned_parent;
  Window* top_level_windows;
  unsigned int num_top_level_windows;
  XQueryTree(display,
             root,
             &returned_root,
             &returned_parent,
             &top_level_windows,
             &num_top_level_windows);

  for (unsigned int i = 0; i < num_top_level_windows; ++i) {
    XWindowAttributes attr;
    XGetWindowAttributes(display, top_level_windows[i], &attr);
    if (attr.map_state == IsViewable) {
      item_get_from_window(top_level_windows[i], True);
    }
  }

  XFree(top_level_windows);
  XUngrabServer(display);
}
