#include "view.h"
#include "xapi.h"
#include "error.h"
#include "list.h"
#include "item_window.h"
#include <limits.h>

Bool debugpicks = False;
Bool debuglayers = False;

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




void view_abstract_draw(View *view, List *items, ItemFilter *filter) {
  if (!items) return;
  for (size_t idx = 0; idx < items->count; idx++) {
    Item *item = (Item *) items->entries[idx];
    if (!filter || filter(item)) {
      if (item_isinstance(item, &item_type_window)) {
        try();
        item->type->draw(view, item);
        XErrorEvent e;
        if (!catch(&e)) {
          if (   (   e.error_code == BadWindow
                  || e.error_code == BadDrawable)
              && e.resourceid == ((ItemWindow *) item)->window) {
            item_remove(item);
          } else {
            throw(&e);
          }
        }
      } else {
        item->type->draw(view, item);
      }     
    }
  }
}

void view_draw(GLint fb, View *view, List *items, ItemFilter *filter) {
  gl_check_error("draw0");
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glEnablei(GL_BLEND, 0);
  glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
  gl_check_error("draw1");
  view->picking = 0;
  view_abstract_draw(view, items, filter);
  gl_check_error("draw2");
}

void view_draw_picking(GLint fb, View *view, List *items, ItemFilter *filter) {
  gl_check_error("view_draw_picking1");
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  view->picking = 1;
  view_abstract_draw(view, items, filter);
  gl_check_error("view_draw_picking2");
}
  
void view_pick(GLint fb, View *view, int x, int y, int *winx, int *winy, Item **returnitem) {
  float data[4];
  memset(data, 0, sizeof(data));
  glReadPixels(x, view->height - y, 1, 1, GL_RGBA, GL_FLOAT, (GLvoid *) data);
  gl_check_error("pick2");
  if (debugpicks) {
    fprintf(stderr, "Pick %d,%d -> %f,%f,%f,%f\n", x, y, data[0], data[1], data[2], data[3]);
  }
  *winx = 0;
  *winy = 0;
  *returnitem = NULL;
  if (data[2] != 0.0) {
    *returnitem = item_get(data[2] * (float) INT_MAX);
    if (*returnitem) {
      *winx = (int) (data[0] * (*returnitem)->width);
      *winy = (int) (data[1] * (*returnitem)->height);
    }
  }
  if (debugpicks) {
    if (*returnitem) {
      fprintf(stderr, "  -> %d,%d,%d\n", (*returnitem)->id, *winx, *winy);
    } else {
      fprintf(stderr, "  -> NULL\n");
    }
 }
}

void view_load_layer(View *view) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(display, root, view->attr_layer, 0, sizeof(Atom), 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
   view->layer = *(Atom *) prop_return;
  }
  XFree(prop_return);
}
void view_load_screen(View *view) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(display, root, view->attr_view, 0, sizeof(float)*4, 0, AnyPropertyType,
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
      view_update(view);
    } else if (view->screen[3] == 0.0) {
      view->screen[3] = view->screen[2] * (float) view->height / (float) view->width;
      view_update(view);
    }
  }
  XFree(prop_return);
}
void view_load_size(View *view) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(display, root, view->attr_size, 0, sizeof(long)*2, 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return != None) {
    view->width = prop_return[0];
    view->height = prop_return[1];
  } else {
    view->width = overlay_attr.width;
    view->height = overlay_attr.height;
    long arr[2];
    arr[0] = view->width;
    arr[1] = view->height;
    XChangeProperty(display, root, view->attr_size, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
  }
  XFree(prop_return);
}

Atom atom_append(Display *display, Atom base, char *suffix) {
  char *strbase = XGetAtomName(display, base);
  char appended[strlen(strbase) + strlen(suffix) + 1];
  strcpy(appended, strbase);
  strcpy(appended + strlen(strbase), suffix);
  XFree(strbase);
  return XInternAtom(display, appended, False);
}

View *view_load(Atom name) {
  View *view = malloc(sizeof(View));
  view->name = name;
  view->attr_layer = atom_append(display, name, "_LAYER");
  view->attr_view = atom_append(display, name, "_VIEW");
  view->attr_size = atom_append(display, name, "_SIZE");
  view_load_size(view);
  view_load_layer(view);
  view_load_screen(view);
  
  return view;
}

List *view_load_all(void) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(display, root, IG_VIEWS, 0, 100000, 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return == None) {
    XFree(prop_return);
    return NULL;
  }
  
  List *res = list_create();
  
  for (int i=0; i < nitems_return; i++) {
    list_append(res, (void *) view_load(((Atom *) prop_return)[i]));
  }
  XFree(prop_return);

  if (debuglayers) {
   for (size_t idx = 0; idx < res->count; idx++) {
     View *v = (View *) res->entries[idx];
     printf("VIEW: layer=%s screen=%f,%f,%f,%f\n",
            XGetAtomName(display, v->layer),
            v->screen[0],
            v->screen[1],
            v->screen[2],
            v->screen[3]);
    }
  }
  
  return res;
}

void view_free(View *view) {
  free(view);
}

void view_free_all(List *views) {
  for (size_t idx = 0; idx < views->count; idx++) {
    free(views->entries[idx]);
  }
  list_destroy(views);
}

void view_update(View *view) {
 if (   (view->_screen[0] != view->screen[0])
     || (view->_screen[1] != view->screen[1])
     || (view->_screen[2] != view->screen[2])
     || (view->_screen[3] != view->screen[3])) {
    long arr[4];
    for (int i = 0; i < 4; i++) {
      *(float *) (arr + i) = view->screen[i];
      view->_screen[i] = view->screen[i];
    }
    XChangeProperty(display, root, view->attr_view, XA_FLOAT, 32, PropModeReplace, (void *) arr, 4);
  }
}

View *view_find(List *views, Atom name) {
  for (size_t idx = 0; idx < views->count; idx++) {
    View *v = (View *) views->entries[idx];   
    if (v->layer == name) {
      return v;
    }
  }
  return NULL;
}
