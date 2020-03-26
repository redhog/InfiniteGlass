#include "glapi.h"
#include "xapi.h"
#include "item.h"
#include "view.h"
#include "shader.h"
#include "wm.h"
#include <limits.h>
#include "property.h"
#include "property_item.h"
#include "debug.h"
#include "rendering.h"
#include <X11/Xatom.h>

List *items_all = NULL;
Item *root_item = NULL;
 
Bool init_items() {
  return True;
}


void item_constructor(XConnection *conn, Item *item, Window window) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return = NULL;
  
  item->window = window;
  item->properties = NULL;
  item->prop_layer = NULL;
  item->prop_item_layer = NULL;
  item->prop_shader = NULL;
  item->prop_size = NULL;
  item->prop_coords = NULL;
  item->prop_coord_types = NULL;
  item->prop_draw_type = NULL;
  item->draw_cycles_left = 0;
  item->parent_item = NULL;
  item->is_clean = False;
  
  if (window == conn->root) {
    Atom layer = ATOM(conn, "IG_LAYER_ROOT");
    XChangeProperty(conn->display, window, ATOM(conn, "IG_LAYER"), XA_ATOM, 32, PropModeReplace, (void *) &layer, 1);
    Atom shader = ATOM(conn, "IG_SHADER_ROOT");
    XChangeProperty(conn->display, window, ATOM(conn, "IG_SHADER"), XA_ATOM, 32, PropModeReplace, (void *) &shader, 1);
    item->is_mapped = True;
  } else {
    XGetWindowProperty(conn->display, window, ATOM(conn, "IG_LAYER"), 0, sizeof(Atom), 0, XA_ATOM,
                       &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
    if (type_return == None) {
      Atom layer = ATOM(conn, "IG_LAYER_DESKTOP");
      XGetWindowAttributes(conn->display, window, &item->attr);
      if (item->attr.override_redirect) {
        layer = ATOM(conn, "IG_LAYER_MENU");
      }
      XChangeProperty(conn->display, window, ATOM(conn, "IG_LAYER"), XA_ATOM, 32, PropModeReplace, (void *) &layer, 1);
    }
    XFree(prop_return);

    long value = 1;
    XChangeProperty(conn->display, window, ATOM(conn, "WM_STATE"), XA_INTEGER, 32, PropModeReplace, (void *) &value, 1);

    XGetWindowAttributes(conn->display, window, &item->attr);
    item->is_mapped = item->attr.map_state == IsViewable; // FIXME: Remove is_mapped...

    XSelectInput(conn->display, window, PropertyChangeMask);
    item->damage = XDamageCreate(conn->display, item->window, XDamageReportNonEmpty);
  }

  XGetWindowProperty(conn->display, window, ATOM(conn, "IG_DRAW_TYPE"), 0, sizeof(Atom), 0, XA_ATOM,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return == None) {
    Atom draw_type = ATOM(conn, "IG_DRAW_TYPE_POINTS");
    XChangeProperty(conn->display, window, ATOM(conn, "IG_DRAW_TYPE"), XA_ATOM, 32, PropModeReplace, (void *) &draw_type, 1);
  }
  XFree(prop_return);
  
  item->properties = properties_load(conn, window);
  item->prop_layer = properties_find(item->properties, ATOM(conn, "IG_LAYER"));
  item->prop_item_layer = properties_find(item->properties, ATOM(conn, "IG_ITEM_LAYER"));
  item->prop_shader = properties_find(item->properties, ATOM(conn, "IG_SHADER"));
  item->prop_size = properties_find(item->properties, ATOM(conn, "IG_SIZE"));
  item->prop_coords = properties_find(item->properties, ATOM(conn, "IG_COORDS"));
  item->prop_coord_types = properties_find(item->properties, ATOM(conn, "IG_COORD_TYPES"));
  item->prop_draw_type = properties_find(item->properties, ATOM(conn, "IG_DRAW_TYPE"));
  
  if (window != conn->root) {
    item_update_space_pos_from_window(conn, item);
  }
  
  item->window_pixmap = 0;
  texture_initialize(&item->window_texture);
}
void item_destructor(XConnection *conn, Item *item) {
  texture_destroy(conn, &item->window_texture);
}
void item_draw_subs(Rendering *rendering) {
  Item *parent_item = rendering->parent_item;
  Item *item = rendering->item;

  if (!item->prop_coords) return;
    
  rendering->parent_item = item;

  rendering->widget_id = 0; // widget_id = 0 is already used for "this is no widget"
  properties_draw(item->properties, rendering);
  if (item != root_item) properties_draw(root_item->properties, rendering);

  rendering->parent_item = parent_item;
  rendering->item = item;
}
void item_draw(Rendering *rendering) {
  XConnection *conn = rendering->conn;

  if (!rendering->item->is_clean) {
    item_update(rendering->conn, rendering->item);
  }
  
  rendering->texture_unit = 0;
  rendering->shader = item_get_shader(conn, rendering->item);
  if (!rendering->shader) return;
  glUseProgram(rendering->shader->program);
  shader_reset_uniforms(rendering->shader);

  if (rendering->item->is_mapped) {
    Item *item = rendering->item;
    Shader *shader = rendering->shader;

    
    if (!rendering->view->picking) {
      if (item->draw_cycles_left > 0) {
        texture_from_pixmap(conn, &item->window_texture, item->window_pixmap);
        item->draw_cycles_left--;
      }
     
      glUniform1i(shader->window_sampler_attr, rendering->texture_unit);
      glActiveTexture(GL_TEXTURE0 + rendering->texture_unit);
      glBindTexture(GL_TEXTURE_2D, item->window_texture.texture_id);
      glBindSampler(rendering->texture_unit, 0);
      rendering->texture_unit++;
    }

    rendering->source_item = root_item;
    properties_to_gl(root_item->properties, "root_", rendering);
    GL_CHECK_ERROR("item_draw_root_properties", "%ld.%s", item->window, rendering->shader->name_str);
    if (rendering->parent_item) {
      rendering->source_item = rendering->parent_item;
      properties_to_gl(rendering->parent_item->properties, "parent_", rendering);
      GL_CHECK_ERROR("item_draw_parent_properties", "%ld.%s", item->window, rendering->shader->name_str);
    }
    rendering->source_item = rendering->item;
    properties_to_gl(rendering->item->properties, "", rendering);
    GL_CHECK_ERROR("item_draw_properties", "%ld.%s", item->window, rendering->shader->name_str);
    
    glUniform1i(shader->picking_mode_attr, rendering->view->picking);
    glUniform4fv(shader->screen_attr, 1, rendering->view->screen);
    glUniform2i(shader->size_attr, rendering->view->width, rendering->view->height);
    glUniform1i(shader->border_width_attr, rendering->item->attr.border_width);
    glUniform2i(shader->pointer_attr, mouse.root_x, rendering->view->height - mouse.root_y);

    DEBUG("setwin", "%ld\n", rendering->item->window);
    if (rendering->parent_item) {
      glUniform1i(shader->window_id_attr, rendering->parent_item->window);
      glUniform1i(shader->widget_id_attr, rendering->widget_id);
    } else {
      glUniform1i(shader->window_id_attr, rendering->item->window);
      glUniform1i(shader->widget_id_attr, 0);
    }
    
    GL_CHECK_ERROR("item_draw2", "%ld.%s", item->window, rendering->shader->name_str);
    
    DEBUG("draw_arrays", "%ld.draw(%ld items)\n", rendering->item->window, rendering->array_length);

    Atom prop_draw_type = ATOM(conn, "IG_DRAW_TYPE_POINTS");
    GLuint draw_type = GL_POINTS;
    if (item->prop_draw_type) {
      prop_draw_type = (Atom) item->prop_draw_type->values.dwords[0];
      if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_POINTS")) draw_type = GL_POINTS;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_LINES")) draw_type = GL_LINES;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_LINE_STRIP")) draw_type = GL_LINE_STRIP;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_LINES_ADJACENCY")) draw_type = GL_LINES_ADJACENCY;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_LINE_STRIP_ADJACENCY")) draw_type = GL_LINE_STRIP_ADJACENCY;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_TRIANGLES")) draw_type = GL_TRIANGLES;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_TRIANGLE_STRIP")) draw_type = GL_TRIANGLE_STRIP;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_TRIANGLE_FAN")) draw_type = GL_TRIANGLE_FAN;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_TRIANGLES_ADJACENCY")) draw_type = GL_TRIANGLES_ADJACENCY;
      else if (prop_draw_type == ATOM(conn, "IG_DRAW_TYPE_TRIANGLE_STRIP_ADJACENCY")) draw_type = GL_TRIANGLE_STRIP_ADJACENCY;
    }  
    glDrawArrays(draw_type, 0, rendering->array_length);

    GL_CHECK_ERROR("item_draw3", "%ld.%s: %s(%ld)",
                   item->window, rendering->shader->name_str,
                   XGetAtomName(conn->display, prop_draw_type), rendering->array_length);

    if (!rendering->view->picking) {
      XErrorEvent error;
      x_try(conn);
      XDamageSubtract(conn->display, item->damage, None, None);
      x_catch(conn, &error);
    }
    item_draw_subs(rendering);
  }
}

void item_update(XConnection *conn, Item *item) {
  if (item->window == conn->root) return;
  if (!item->is_mapped) return;
  item->_is_mapped = item->is_mapped;
  item->is_clean = True;

  x_push_error_context(conn, "item_update_pixmap");
  
  // FIXME: free all other stuff if already created

  if (item->window_pixmap) {
    XFreePixmap(conn->display, item->window_pixmap);
  }
  item->window_pixmap = XCompositeNameWindowPixmap(conn->display, item->window);

  if (DEBUG_ENABLED("window.update")) {
    Window root_return;
    int x_return;
    int y_return;
    unsigned int width_return;
    unsigned int height_return;
    unsigned int border_width_return;
    unsigned int depth_return;

    int res = XGetGeometry(conn->display, item->window_pixmap,
                           &root_return, &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return);
    DEBUG("window.update",
          "XGetGeometry(pixmap=%lu) = status=%d, root=%lu, x=%u, y=%u, w=%u, h=%u border=%u depth=%u\n",
           item->window_pixmap, res, root_return, x_return, y_return, width_return, height_return, border_width_return, depth_return);
  }
  
  texture_from_pixmap(conn, &item->window_texture, item->window_pixmap);

  GL_CHECK_ERROR("item_update_pixmap2", "%ld", item->window);

  x_pop_error_context(conn);  
}

void item_trigger_update(Item *item) {
  item->is_clean = False;
}

Bool item_properties_update(XConnection *conn, Item *item, PropertyFetch *fetch) {
  Bool res = properties_update(conn, item->properties, fetch);
  if (res) {
    if (fetch->name == ATOM(conn, "IG_LAYER") && !item->prop_layer) item->prop_layer = properties_find(item->properties, ATOM(conn, "IG_LAYER"));
    if (fetch->name == ATOM(conn, "IG_ITEM_LAYER") && !item->prop_item_layer) item->prop_item_layer = properties_find(item->properties, ATOM(conn, "IG_ITEM_LAYER"));
    if (fetch->name == ATOM(conn, "IG_SHADER") && !item->prop_shader) item->prop_shader = properties_find(item->properties, ATOM(conn, "IG_SHADER"));
    if (fetch->name == ATOM(conn, "IG_SIZE") && !item->prop_size) item->prop_size = properties_find(item->properties, ATOM(conn, "IG_SIZE"));
    if (fetch->name == ATOM(conn, "IG_COORDS") && !item->prop_coords) item->prop_coords = properties_find(item->properties, ATOM(conn, "IG_COORDS"));
    if (fetch->name == ATOM(conn, "IG_COORD_TYPES") && !item->prop_coord_types) item->prop_coord_types = properties_find(item->properties, ATOM(conn, "IG_COORD_TYPES"));
    if (fetch->name == ATOM(conn, "IG_DRAW_TYPE") && !item->prop_draw_type) item->prop_draw_type = properties_find(item->properties, ATOM(conn, "IG_DRAW_TYPE"));
  }
  return res;
}

Shader *item_get_shader(XConnection *conn, Item *item) {
  Atom shader = ATOM(conn, "IG_SHADER_DEFAULT");
  if (item->prop_shader) shader = item->prop_shader->values.dwords[0];
  return shader_find(shaders, shader);
}
void item_print(XConnection *conn, Item *item) {
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
  properties_print(conn, item->properties, stderr);
}

Item *item_create(XConnection *conn, Window window) {
  Item *item = (Item *) malloc(sizeof(Item));

  item->is_mapped = False;
  item->_is_mapped = False;
  item_constructor(conn, item, window);
  item_add(item);
  return item;
}

void item_add(Item *item) {
  if (!items_all) items_all = list_create();
  list_append(items_all, (void *) item);
}

void item_remove(XConnection *conn, Item *item) {
  list_remove(items_all, (void *) item);
  item_destructor(conn, item);
}

void item_update_space_pos_from_window(XConnection *conn, Item *item) {
  if (item->window == conn->root) return;
  
  XGetWindowAttributes(conn->display, item->window, &item->attr);

  item->x                   = item->attr.x;
  item->y                   = item->attr.y;
  int width                 = item->attr.width;
  int height                = item->attr.height;
  DEBUG("window.spacepos", "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, item->x, item->y, width, height);

  long arr[2] = {width, height};
  DEBUG("set_ig_size", "%ld.Setting IG_SIZE = %d,%d\n", item->window, width, height);
  XChangeProperty(conn->display, item->window, ATOM(conn, "IG_SIZE"), XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
  
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;
  XGetWindowProperty(conn->display, item->window, ATOM(conn, "IG_COORDS"), 0, sizeof(float)*4, 0, AnyPropertyType,
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
      DEBUG("position", "XXX %f*%d/%d=%f, %f*%d/%d=%f\n", v->screen[2], width, v->width, coords[2], v->screen[3], height, v->height, coords[3]);
    } else {
      coords[0] = ((float) (item->x - conn->overlay_attr.x)) / (float) conn->overlay_attr.width;
      coords[1] = ((float) (conn->overlay_attr.height - item->y - conn->overlay_attr.y)) / (float) conn->overlay_attr.width;
      coords[2] = ((float) (width)) / (float) conn->overlay_attr.width;
      coords[3] = ((float) (height)) / (float) conn->overlay_attr.width;
      DEBUG("position", "Setting item position to 0 for %ld (IG_COORDS & IG_LAYER missing).\n", item->window);
    }

    if (item->prop_layer && item->prop_layer->values.dwords && (Atom) item->prop_layer->values.dwords[0] == ATOM(conn, "IG_LAYER_MENU")) {
      DEBUG("menu.setup", "%ld: %d,%d[%d,%d]   %f,%f,%f,%f\n",
            item->window,
            item->attr.x, item->attr.y, item->attr.width, item->attr.height,
            coords[0],coords[1],coords[2],coords[3]);
    }

    DEBUG("set_ig_coords", "%ld.Setting IG_COORDS = %f,%f[%f,%f]\n", item->window, coords[0], coords[1], coords[2], coords[3]);
    long coords_arr[4];
    for (int i = 0; i < 4; i++) {
      coords_arr[i] = *(long *) &coords[i];
    }
    XChangeProperty(conn->display, item->window, ATOM(conn, "IG_COORDS"), XA_FLOAT, 32, PropModeReplace, (void *) coords_arr, 4);
  }
}

Item *item_get_from_window(XConnection *conn, Window window, int create) {
  if (items_all) {
    for (size_t idx = 0; idx < items_all->count; idx++) {
      Item *item = (Item *) items_all->entries[idx];
      if (item->window == window) return item;
    }
  }
  if (!create) return NULL;
  
  DEBUG("window.add", "Adding window %ld\n", window);
  return item_create(conn, window);
}

Item *item_get_from_widget(XConnection *conn, Item *parent, int widget) {
  if (!parent || !widget) return parent;
  widget--;
  if (widget < parent->properties->properties->count) {
    Property *prop = (Property *) parent->properties->properties->entries[widget];
    if (prop->type_handler != &property_item) return NULL;
    return item_get_from_window(conn, (Window) prop->values.dwords[0], False);
  }
  widget -= parent->properties->properties->count;
  if ((parent != root_item) && (widget < root_item->properties->properties->count)) {
    Property *prop = (Property *) root_item->properties->properties->entries[widget];
    if (prop->type_handler != &property_item) return NULL;
    return item_get_from_window(conn, (Window) prop->values.dwords[0], False);
  }
  return NULL;
}

void items_get_from_toplevel_windows(XConnection *conn) {
  XCompositeRedirectSubwindows(conn->display, conn->root, CompositeRedirectAutomatic);

  root_item = item_get_from_window(conn, conn->root, True);
  
  XGrabServer(conn->display);
  
  Window returned_root, returned_parent;
  Window* top_level_windows;
  unsigned int num_top_level_windows;
  XQueryTree(conn->display,
             conn->root,
             &returned_root,
             &returned_parent,
             &top_level_windows,
             &num_top_level_windows);

  for (unsigned int i = 0; i < num_top_level_windows; ++i) {
    XWindowAttributes attr;
    XGetWindowAttributes(conn->display, top_level_windows[i], &attr);
    if (attr.map_state == IsViewable) {
      item_get_from_window(conn, top_level_windows[i], True);
    }
  }

  XFree(top_level_windows);
  XUngrabServer(conn->display);
}

Bool item_filter_by_layer(void *data, Item *item) {
  Atom current_layer = *(Atom *) data;
  return item->prop_layer && item->prop_layer->values.dwords && (Atom) item->prop_layer->values.dwords[0] == current_layer;
}
