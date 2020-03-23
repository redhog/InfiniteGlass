#include "view.h"
#include "xapi.h"
#include "error.h"
#include "list.h"
#include "item.h"
#include "debug.h"
#include <limits.h>
#include <math.h>

Bool debug_picking = False;

List *views = NULL;

Bool init_view(void) {
  return True;
}

void mat4mul(float *mat4, float *vec4, float *outvec4) {
  for (int i = 0; i < 4; i++) {
   float res = 0.0;
    for (int j = 0; j < 4; j++) {
      res += mat4[i*4 + j] * vec4[j];
    }
    outvec4[i] = res;
  }
  /*
  printf("|%f,%f,%f,%f||%f|   |%f|\n"
         "|%f,%f,%f,%f||%f|   |%f|\n"
         "|%f,%f,%f,%f||%f| = |%f|\n"
         "|%f,%f,%f,%f||%f|   |%f|\n",
         mat4[0], mat4[1], mat4[2], mat4[3],  vec4[0], outvec4[0],
         mat4[4], mat4[5], mat4[6], mat4[7],  vec4[1], outvec4[1],
         mat4[8], mat4[9], mat4[10],mat4[11], vec4[2], outvec4[2],
         mat4[12],mat4[13],mat4[14],mat4[15], vec4[3], outvec4[3]);
  */
}

void view_to_space(View *view, float screenx, float screeny, float *spacex, float *spacey) {
  float screen2space[4*4] = {view->screen[2]/view->width,0,0,view->screen[0],
                             0,-view->screen[3]/view->height,0,view->screen[1],
                             0,0,1,0,
                             0,0,0,1};
  float space[4] = {screenx, screeny, 0., 1.};
  float outvec[4];
  mat4mul(screen2space, space, outvec);
  *spacex = outvec[0];
  *spacey = outvec[1];
}
void view_from_space(View *view, float spacex, float spacey, float *screenx, float *screeny) {
  float space2screen[4*4] = {view->width/view->screen[2], 0., 0., -view->width*view->screen[0]/view->screen[2],
                             0., -view->height/view->screen[3], 0., -view->height*view->screen[1]/view->screen[3],
                             0., 0., 1., 0.,
                             0., 0., 0., 1.};
  float space[4] = {spacex, spacey, 0., 1.};
  float outvec[4];
  mat4mul(space2screen, space, outvec);
  *screenx = outvec[0];
  *screeny = outvec[1];
}

void view_abstract_draw(XConnection *conn, View *view, List *items, ItemFilter *filter) {
  Rendering rendering;
  rendering.conn = conn;
  rendering.shader = NULL;
  rendering.view = view;
  rendering.array_length = 1;
  
  List *to_delete = NULL;
  if (!items) return;
  for (size_t idx = 0; idx < items->count; idx++) {
    Item *item = (Item *) items->entries[idx];
    if (!filter || filter->fn(filter->data, item)) {
      try(conn);
      rendering.parent_item = NULL;
      rendering.item = item;
      item_draw(&rendering);
      XErrorEvent e;
      if (!catch(conn, &e)) {
        if (   (   e.error_code == BadWindow
                || e.error_code == BadDrawable)
            && e.resourceid == item->window) {
          if (!to_delete) to_delete = list_create();
          list_append(to_delete, item);
        } else {
          throw(conn, &e);
        }
      }
    }
  }
  if (to_delete) {
    for (size_t idx = 0; idx < to_delete->count; idx++) {
      item_remove(conn, (Item *) to_delete->entries[idx]);
    }
    list_destroy(to_delete);
  }
}

void view_draw(XConnection *conn, GLint fb, View *view, List *items, ItemFilter *filter) {
  GL_CHECK_ERROR("draw0", "%s", XGetAtomName(conn->display, view->name));
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glEnablei(GL_BLEND, 0);
  glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
  GL_CHECK_ERROR("draw1", "%s", XGetAtomName(conn->display, view->name));
  view->picking = debug_picking;
  view_abstract_draw(conn, view, items, filter);
  GL_CHECK_ERROR("draw2", "%s", XGetAtomName(conn->display, view->name));
}

void view_draw_picking(XConnection *conn, GLint fb, View *view, List *items, ItemFilter *filter) {
  GL_CHECK_ERROR("view_draw_picking1", "%s", XGetAtomName(conn->display, view->name));
  glBindFramebuffer(GL_FRAMEBUFFER, fb);

  glEnablei(GL_BLEND, 0);
  glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
  view->picking = 1;
  view_abstract_draw(conn, view, items, filter);
  GL_CHECK_ERROR("view_draw_picking2", "%s", XGetAtomName(conn->display, view->name));
}
  
void view_pick(XConnection *conn, GLint fb, View *view, int x, int y, int *winx, int *winy, Item **item, Item **parent_item) {
  float data[4];
  Window window = None;
  int widget = 0;
  memset(data, 0, sizeof(data));
  glReadPixels(x, view->height - y, 1, 1, GL_RGBA, GL_FLOAT, (GLvoid *) data);
  GL_CHECK_ERROR("pick2", "%s", XGetAtomName(conn->display, view->name));
  DEBUG("pick", "Pick %d,%d -> %f,%f,%f,%f\n", x, y, data[0], data[1], data[2], data[3]);
  *winx = 0;
  *winy = 0;
  *item = NULL;
  *parent_item = 0;
  if (data[2] >= 0.0) {
    // See glass-theme/glass_theme/shaders/lib/fragment_picking.glsl
    // for the encoding of this
    window = (Window) (((0b111111 & (int) data[2]) << 23) + (int) data[3]);
    widget = (((int) data[2]) >> 6);
    *item = item_get_from_window(conn, window, False);
    if (item && widget) {
      *parent_item = *item;
      *item = item_get_from_widget(conn, *item, widget);
    }
    if (*item && (*item)->prop_size) {
      unsigned long width = (*item)->prop_size->values.dwords[0];
      unsigned long height = (*item)->prop_size->values.dwords[1];
      
      *winx = (int) (data[0] * width);
      *winy = (int) (data[1] * height);
    }
  }
  if (widget) {
    DEBUG("pick", "  -> %d%s/%d%s,%d,%d\n", window, *parent_item ? "" : "(unknown!)", widget, *item ? "" : "(unknown!)", *winx, *winy);
  } else {
    DEBUG("pick", "  -> %d%s,%d,%d\n", window, *item ? "" : "(unknown!)", *winx, *winy);
  }
}

void view_load_layer(XConnection *conn, View *view) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(conn->display, conn->root, view->attr_layer, 0, sizeof(Atom) * 1000, 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    view->layers = (Atom *) prop_return;
    view->nr_layers = nitems_return;
    view->layers_str = malloc(sizeof(char *) * nitems_return);
    for (size_t idx = 0; idx < view->nr_layers; idx++) {
      view->layers_str[idx] = XGetAtomName(conn->display, view->layers[idx]);
    }
  }
}
void view_load_screen(XConnection *conn, View *view) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(conn->display, conn->root, view->attr_view, 0, sizeof(float)*4, 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    for (int i = 0; i < 4; i++) {
      // Yes, on 64bit linux, long is 8 bytes, and XGetWindowProperty inserts a bunch of zeroes!
      // So we need to step through the array in chunks of long, not float (which is still 4 bytes)...
      view->screen[i] = *(float *) &((long *) prop_return)[i];
      view->_screen[i] = *(float *) &((long *) prop_return)[i];
    }
    if (view->screen[2] == 0.0) {
      view->screen[2] = view->screen[3] * (float) view->width / (float) view->height;     
      view_update(conn, view);
    } else if (view->screen[3] == 0.0) {
      view->screen[3] = view->screen[2] * (float) view->height / (float) view->width;
      view_update(conn, view);
    }
  }
  XFree(prop_return);
}
void view_load_size(XConnection *conn, View *view) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(conn->display, conn->root, view->attr_size, 0, sizeof(long)*2, 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    view->width = ((long *) prop_return)[0];
    view->height = ((long *) prop_return)[1];
  } else {
    view->width = conn->overlay_attr.width;
    view->height = conn->overlay_attr.height;
    long arr[2];
    arr[0] = view->width;
    arr[1] = view->height;
    XChangeProperty(conn->display, conn->root, view->attr_size, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
  }
  XFree(prop_return);
}

View *view_load(XConnection *conn, Atom name) {
  View *view = malloc(sizeof(View));
  view->layers = NULL;
  view->nr_layers = 0;
  view->name = name;
  view->name_str = XGetAtomName(conn->display, name);
  view->attr_layer = atom_append(conn->display, name, "_LAYER");
  view->attr_view = atom_append(conn->display, name, "_VIEW");
  view->attr_size = atom_append(conn->display, name, "_SIZE");
  view_load_size(conn, view);
  view_load_layer(conn, view);
  view_load_screen(conn, view);
  
  return view;
}

List *view_load_all(XConnection *conn) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(conn->display, conn->root, ATOM(conn, "IG_VIEWS"), 0, 100000, 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return == None) {
    XFree(prop_return);
    return NULL;
  }
  
  List *res = list_create();
  
  for (int i=0; i < nitems_return; i++) {
    list_append(res, (void *) view_load(conn, ((Atom *) prop_return)[i]));
  }
  XFree(prop_return);

  if (DEBUG_ENABLED("layers")) {
   for (size_t idx = 0; idx < res->count; idx++) {
     View *v = (View *) res->entries[idx];
     
     DEBUG("layers",
           "VIEW: view=%s screen=%f,%f,%f,%f\n",
           XGetAtomName(conn->display, v->name),
           v->screen[0],
           v->screen[1],
           v->screen[2],
           v->screen[3]);
    }
  }
  
  return res;
}

void view_free(View *view) {
  XFree(view->name_str);
  if (view->layers) {
    XFree(view->layers);
    for (size_t idx = 0; idx < view->nr_layers; idx++) {
      XFree(view->layers_str[idx]);
    }
    free(view->layers_str);
  }
  free(view);
}

void view_free_all(List *views) {
  if (!views) return;
  for (size_t idx = 0; idx < views->count; idx++) {
    free(views->entries[idx]);
  }
  list_destroy(views);
}

void view_update(XConnection *conn, View *view) {
 if (   (view->_screen[0] != view->screen[0])
     || (view->_screen[1] != view->screen[1])
     || (view->_screen[2] != view->screen[2])
     || (view->_screen[3] != view->screen[3])) {
    long arr[4];
    for (int i = 0; i < 4; i++) {
      *(float *) (arr + i) = view->screen[i];
      view->_screen[i] = view->screen[i];
    }
    XChangeProperty(conn->display, conn->root, view->attr_view, XA_FLOAT, 32, PropModeReplace, (void *) arr, 4);
  }
}

View *view_find(List *views, Atom name) {
  if (!views) return NULL;
  for (size_t idx = 0; idx < views->count; idx++) {
    View *v = (View *) views->entries[idx];
    for (size_t layer_idx = 0; layer_idx < v->nr_layers; layer_idx++) {
      if (v->layers[layer_idx] == name) {
        return v;
      }
    }
  }
  return NULL;
}

void view_print(View *v) {
  printf("%s: layers=", v->name_str);
  for (size_t idx = 0; idx < v->nr_layers; idx++) {
    printf("%s%s", idx == 0 ? "" : ",", v->layers_str[idx]);
  }  
  printf(" screen=%f,%f,%f,%f size=%d,%d\n",
         v->screen[0],
         v->screen[1],
         v->screen[2],
         v->screen[3],
         v->width,
         v->height);
}

Bool views_update(XConnection *conn, Atom atom) {
  Bool handled = False;
  if (atom == ATOM(conn, "IG_VIEWS")) {
    view_free_all(views);
    views = view_load_all(conn);
    handled = True;
  } else {
    if (views) {
      for (size_t idx = 0; idx < views->count; idx++) {
        View *v = (View *) views->entries[idx];
        if (atom == v->attr_layer) {
          view_load_layer(conn, v);
          handled=True;
        } else if (atom == v->attr_view) {
          view_load_screen(conn, v);
          handled=True;
        }
      }
    }
  }
  return handled;
}
