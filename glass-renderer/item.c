#include "glapi.h"
#include "xapi.h"
#include "item.h"
#include "view.h"
#include "shader.h"
#include "wm.h"
#include <limits.h>
#include "property.h"
#include "property_item.h"
#include "property_coords.h"
#include "debug.h"
#include "rendering.h"
#include "mainloop.h"
#include "texture.h"
#include <X11/Xatom.h>

List *items_all = NULL;
Item *root_item = NULL;
 
Bool init_items() {
  return True;
}


void item_constructor(Item *item) {
  item->window = None;
  item->is_mapped = False;
  item->_is_mapped = False;
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
  item->attr = NULL;
  item->geom = NULL;
  texture_initialize(&item->window_texture);
}

void item_update_space_pos_from_window_load(Item *item, xcb_get_property_reply_t *reply, xcb_generic_error_t *error) {
  item->x                   = item->geom->x;
  item->y                   = item->geom->y;
  int width                 = item->geom->width;
  int height                = item->geom->height;
  DEBUG("window.spacepos", "Spacepos for %ld is %d,%d [%d,%d]\n", item->window, item->x, item->y, width, height);

  long arr[2] = {width, height};
  DEBUG("set_ig_size", "%ld.Setting IG_SIZE = %d,%d\n", item->window, width, height);
  xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, item->window, ATOM("IG_SIZE"), XA_INTEGER, 32, 2, (void *) arr);    

  if (!reply->type) {
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
      coords[0] = ((float) (item->x - overlay_attr.x)) / (float) overlay_attr.width;
      coords[1] = ((float) (overlay_attr.height - item->y - overlay_attr.y)) / (float) overlay_attr.width;
      coords[2] = ((float) (width)) / (float) overlay_attr.width;
      coords[3] = ((float) (height)) / (float) overlay_attr.width;
      DEBUG("position", "Setting item position to 0 for %ld (IG_COORDS & IG_LAYER missing).\n", item->window);
    }

    if (item->prop_layer && item->prop_layer->values.dwords && (Atom) item->prop_layer->values.dwords[0] == ATOM("IG_LAYER_MENU")) {
      DEBUG("menu.setup", "%ld: %d,%d[%d,%d]   %f,%f,%f,%f\n",
            item->window,
            item->geom->x, item->geom->y, item->geom->width, item->geom->height,
            coords[0],coords[1],coords[2],coords[3]);
    }

    DEBUG("set_ig_coords", "%ld.Setting IG_COORDS = %f,%f[%f,%f]\n", item->window, coords[0], coords[1], coords[2], coords[3]);
    xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, item->window, ATOM("IG_COORDS"), XA_FLOAT, 32, 4, (void *) coords);
  }
  free(reply);
}
void item_update_space_pos_from_window(Item *item) {
  xcb_get_property_cookie_t cookie = xcb_get_property(xcb_display, 0, item->window, ATOM("IG_COORDS"), AnyPropertyType, 0, 1000000000);
  MAINLOOP_XCB_DEFER(cookie, &item_update_space_pos_from_window_load, (void *) item);
}

void item_initialize_draw_type_load(Item *item, xcb_get_property_reply_t *reply, xcb_generic_error_t *error) {
  if (reply->type == None) {
    Atom draw_type = ATOM("IG_DRAW_TYPE_POINTS");
    xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, item->window, ATOM("IG_DRAW_TYPE"), XA_ATOM, 32, 1, (void *) &draw_type);    
  }
  free(reply);
  
  item->properties = properties_load(item->window);
  item->prop_layer = properties_find(item->properties, ATOM("IG_LAYER"));
  item->prop_item_layer = properties_find(item->properties, ATOM("IG_ITEM_LAYER"));
  item->prop_shader = properties_find(item->properties, ATOM("IG_SHADER"));
  item->prop_size = properties_find(item->properties, ATOM("IG_SIZE"));
  item->prop_coords = properties_find(item->properties, ATOM("IG_COORDS"));
  item->prop_coord_types = properties_find(item->properties, ATOM("IG_COORD_TYPES"));
  item->prop_draw_type = properties_find(item->properties, ATOM("IG_DRAW_TYPE"));
  
  if (item->window != root) {
    item_update_space_pos_from_window(item);
  }
  
  item->window_pixmap = 0;
  texture_initialize(&item->window_texture);

  item_update(item);
}
void item_initialize_draw_type(Item *item) {
  xcb_get_property_cookie_t cookie = xcb_get_property(xcb_display, 0, item->window, ATOM("IG_DRAW_TYPE"), XA_ATOM, 0, 1000000000);
  MAINLOOP_XCB_DEFER(cookie, &item_initialize_draw_type_load, (void *) item);
}

void item_initialize_layer_load(Item *item, xcb_get_property_reply_t *reply, xcb_generic_error_t *error) {
  if (reply->type == None) {
    Atom layer = ATOM("IG_LAYER_DESKTOP");
    if (item->attr->override_redirect) {
      layer = ATOM("IG_LAYER_MENU");
    }
    xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, item->window, ATOM("IG_LAYER"), XA_ATOM, 32, 1, (void *) &layer);    
  }
  free(reply);
  
  item->is_mapped = item->attr->map_state == IsViewable; // FIXME: Remove is_mapped...
  
  uint32_t values[] = {PropertyChangeMask};
  xcb_change_window_attributes(xcb_display, item->window, XCB_CW_EVENT_MASK, values);

  item->damage = xcb_generate_id(xcb_display);
  xcb_damage_create(xcb_display, item->damage, item->window, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);
  
  item_initialize_draw_type(item);
}
void item_initialize_layer(Item *item) {
  xcb_get_property_cookie_t cookie = xcb_get_property(xcb_display, 0, item->window, ATOM("IG_LAYER"), XA_ATOM, 0, 1000000000);
  MAINLOOP_XCB_DEFER(cookie, &item_initialize_layer_load, (void *) item);
}

void item_initialize_attr_load(Item *item, xcb_get_window_attributes_reply_t *reply, xcb_generic_error_t *error) {
  item->attr = reply;
  if (item->window == root) {
    Atom layer = ATOM("IG_LAYER_ROOT");
    xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, item->window, ATOM("IG_LAYER"), XA_ATOM, 32, 1, (void *) &layer);    
    Atom shader = ATOM("IG_SHADER_ROOT");
    xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, item->window, ATOM("IG_SHADER"), XA_ATOM, 32, 1, (void *) &shader);    
    item->is_mapped = True;
    item_initialize_draw_type(item);
  } else {
    long value = 1;
    xcb_change_property(xcb_display, XCB_PROP_MODE_REPLACE, item->window, ATOM("WM_STATE"), XA_INTEGER, 32, 1, (void *) &value);    
    item_initialize_layer(item);
  }
}
void item_initialize_attr(Item *item) {
  xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(xcb_display, item->window);
  MAINLOOP_XCB_DEFER(cookie, &item_initialize_attr_load, (void *) item);
}

void item_initialize_geom_load(Item *item, xcb_get_geometry_reply_t *reply, xcb_generic_error_t *error) {
  item->geom = reply;
  item_initialize_attr(item);
}
void item_initialize_geom(Item *item) {
  xcb_get_geometry_cookie_t cookie = xcb_get_geometry(xcb_display, item->window);
  MAINLOOP_XCB_DEFER(cookie, &item_initialize_geom_load, (void *) item);
}


void item_initialize(Item *item, Window window) {
  item->window = window;
  item_initialize_geom(item);
}

void item_destructor(Item *item) {
  if (item->attr) free(item->attr);
  if (item->geom) free(item->geom);
  texture_destroy(&item->window_texture);
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
  rendering->texture_unit = 0;
  rendering->shader = item_get_shader(rendering->item);
  if (!rendering->shader) return;
  glUseProgram(rendering->shader->program);
  shader_reset_uniforms(rendering->shader);

  if (rendering->item->is_mapped) {
    Item *item = rendering->item;
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
    glUniform1i(shader->border_width_attr, rendering->item->geom->border_width);
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
    item_draw_subs(rendering);
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

void item_properties_update(Item *item, Atom name) {
  properties_update(item->properties, name);
  if (name == ATOM("IG_LAYER") && !item->prop_layer) item->prop_layer = properties_find(item->properties, ATOM("IG_LAYER"));
  if (name == ATOM("IG_ITEM_LAYER") && !item->prop_item_layer) item->prop_item_layer = properties_find(item->properties, ATOM("IG_ITEM_LAYER"));
  if (name == ATOM("IG_SHADER") && !item->prop_shader) item->prop_shader = properties_find(item->properties, ATOM("IG_SHADER"));
  if (name == ATOM("IG_SIZE") && !item->prop_size) item->prop_size = properties_find(item->properties, ATOM("IG_SIZE"));
  if (name == ATOM("IG_COORDS") && !item->prop_coords) item->prop_coords = properties_find(item->properties, ATOM("IG_COORDS"));
  if (name == ATOM("IG_COORD_TYPES") && !item->prop_coord_types) item->prop_coord_types = properties_find(item->properties, ATOM("IG_COORD_TYPES"));
  if (name == ATOM("IG_DRAW_TYPE") && !item->prop_draw_type) item->prop_draw_type = properties_find(item->properties, ATOM("IG_DRAW_TYPE"));
}

Shader *item_get_shader(Item *item) {
  Atom shader = ATOM("IG_SHADER_DEFAULT");
  if (item->prop_shader) shader = item->prop_shader->values.dwords[0];
  return shader_find(shaders, shader);
}
void item_print(Item *item) {
  printf("item(%ld):%s ",
         item->window,
         item->is_mapped ? "" : " invisible");
  if (item->prop_size) {
    long width = item->prop_size->values.dwords[0];
    long height = item->prop_size->values.dwords[1];
    printf("[%ld,%ld] @ ", width, height);
  } else {
    printf("[<unknown>] @ ");
  }
  if (item->prop_coords) {
    PropertyCoords *data = (PropertyCoords *) item->prop_coords->data;
    printf("%f,%f[%f,%f]\n",
           data->ccoords[0],
           data->ccoords[1],
           data->ccoords[2],
           data->ccoords[3]);
  } else {
    printf("<unknown>\n");
  }
  properties_print(item->properties, stdout);
}

Item *item_create(Window window) {
  Item *item = (Item *) malloc(sizeof(Item));
  item_constructor(item);
  item_add(item);
  item_initialize(item, window);
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

Item *item_get_from_widget(Item *parent, int widget) {
  if (!parent || !widget) return parent;
  widget--;
  if (widget < parent->properties->properties->count) {
    Property *prop = (Property *) parent->properties->properties->entries[widget];
    if (prop->type_handler != &property_item) return NULL;
    return item_get_from_window((Window) prop->values.dwords[0], False);
  }
  widget -= parent->properties->properties->count;
  if ((parent != root_item) && (widget < root_item->properties->properties->count)) {
    Property *prop = (Property *) root_item->properties->properties->entries[widget];
    if (prop->type_handler != &property_item) return NULL;
    return item_get_from_window((Window) prop->values.dwords[0], False);
  }
  return NULL;
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
