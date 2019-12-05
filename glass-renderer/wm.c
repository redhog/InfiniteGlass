#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/select.h>

#include "xapi.h"
#include "xevent.h"
#include "view.h"
#include "xevent.h"
#include "wm.h"
#include "event.h"
#include "selection.h"
#include "list.h"
#include "debug.h"
#include "fps.h"
#include "property.h"
#include "property_atom.h"
#include "property_window.h"
#include "property_int.h"
#include "property_float.h"
#include "property_svg.h"
#include "property_wm_hints_icon.h"
#include "property_net_wm_icon.h"
#include <X11/extensions/XInput2.h>
#include <SOIL/SOIL.h>
#include <backtrace.h>

struct backtrace_state *trace_state;

List *views = NULL;
List *shaders = NULL;
GLuint picking_fb;

Atom current_layer;
Bool filter_by_layer(Item *item) {
  return item->layer == current_layer;
}

void draw() {
  draw_fps_start();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(1.0, 1.0, 1., 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  for (size_t idx = 0; idx < views->count; idx++) {
    View *v = (View *) views->entries[idx];
    current_layer = v->layer;
    view_draw(0, v, items_all, &filter_by_layer);
  }
  glFlush();
  glXSwapBuffers(display, overlay);
  draw_fps();
}

void pick(int x, int y, int *winx, int *winy, Item **item) {
  View *view = (View *) views->entries[0];
  gl_check_error("pick1");
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  glEnable(GL_SCISSOR_TEST);
  glScissor(x, view->height - y, 1, 1);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  for (size_t idx = 0; idx < views->count; idx++) {
    View *v = (View *) views->entries[idx];
    current_layer = v->layer;
    view_draw_picking(picking_fb, v, items_all, &filter_by_layer);
  }
  glFlush();
  view_pick(picking_fb, view, x, y, winx, winy, item);
}

int init_picking() {
  GLuint color_tex;
  GLuint depth_rb;
  
  glGenTextures(1, &color_tex);
  glBindTexture(GL_TEXTURE_2D, color_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, overlay_attr.width, overlay_attr.height, 0, GL_RGBA, GL_FLOAT, NULL);
  glGenFramebuffers(1, &picking_fb);
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
  glGenRenderbuffers(1, &depth_rb);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, overlay_attr.width, overlay_attr.height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "Unable to create picking framebuffer.\n");
    return 0;
  }
  return 1;
}

Bool selection_sn_handler(Selection *selection, XEvent *event) {
  return False;
}
void selection_sn_clear(Selection *selection) {
}

Bool main_event_handler_function(EventHandler *handler, XEvent *event) {
  XGenericEventCookie *cookie = &event->xcookie;

  unsigned long start_time = get_timestamp();

  gl_check_error("loop");

  if (event->type == PropertyNotify) {
    Bool changed = True;
    Item *item = (Item *) item_get_from_window(event->xproperty.window, False);

    if (item) {
      if (!properties_update(item->properties, event->xproperty.atom)) {
        changed = False;
      }
      if (event->xproperty.atom == IG_SHADER && !item->prop_shader) item->prop_shader = properties_find(item->properties, IG_SHADER);
      if (event->xproperty.atom == IG_SIZE && !item->prop_size) item->prop_size = properties_find(item->properties, IG_SIZE);
      if (event->xproperty.atom == IG_COORDS && !item->prop_coords) item->prop_coords = properties_find(item->properties, IG_COORDS);        
    }

    if (changed) {
      if (event->xproperty.window != root && item && event->xproperty.atom == IG_SIZE) {
        Atom type_return;
        int format_return;
        unsigned long nitems_return;
        unsigned long bytes_after_return;
        unsigned char *prop_return;
        XGetWindowProperty(display, event->xproperty.window, IG_SIZE, 0, sizeof(long)*2, 0, AnyPropertyType,
                           &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
        if (type_return != None) {
          XWindowChanges values;
          values.width = ((long *) prop_return)[0];
          values.height = ((long *) prop_return)[1];
          XConfigureWindow(display, item->window, CWWidth | CWHeight, &values);
          DEBUG("event.size", "SIZE CHANGED TO %i,%i\n", values.width, values.height);
          item->type->update((Item *) item);
        }
        XFree(prop_return);
      } else if (event->xproperty.window == root && event->xproperty.atom == IG_VIEWS) {
        view_free_all(views);
        views = view_load_all();
      } else if (event->xproperty.window == root) {
        Bool handled = False;
        for (size_t idx = 0; idx < views->count; idx++) {
          View *v = (View *) views->entries[idx];
          if (event->xproperty.atom == v->attr_layer) {
            view_load_layer(v);
            handled=True;
          } else if (event->xproperty.atom == v->attr_view) {
            view_load_screen(v);
            handled=True;
          }
        }
        if (!handled) {
          if (DEBUG_ENABLED("event.other")) {
            DEBUG("event.other", "Ignored property event ");
            print_xevent(stderr, display, event);
          }
        }
      }
      draw();
    }
  } else if (cookie->type == GenericEvent) {
    if (XGetEventData(display, cookie)) {
      if (cookie->evtype == XI_RawMotion) {
        // XIRawEvent *re = (XIRawEvent *) cookie->data;
        Window       root_ret, child_ret;
        int          root_x, root_y;
        int          win_x, win_y;
        unsigned int mask;
        XQueryPointer(display, root,
                      &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);

        int winx, winy;
        Item *item;

        pick(root_x, root_y, &winx, &winy, &item);
        if (item && item->layer != IG_LAYER_MENU && item_isinstance(item, &item_type_base)) {
          XWindowChanges values;
          values.x = root_x - winx;
          values.y = root_y - winy;
          values.stack_mode = Above;
          if (values.x != item->x || values.y != item->y) {
            XConfigureWindow(display, item->window, CWX | CWY | CWStackMode, &values);
            item->x = values.x;
            item->y = values.y;
          }

          DEBUG("position", "Point %d,%d -> %lu,%d,%d\n", event->xmotion.x_root, event->xmotion.y_root, item->window, winx, winy);
        } else {
          DEBUG("position", "Point %d,%d -> NONE\n", event->xmotion.x_root, event->xmotion.y_root);
        }
      } else {
        DEBUG("event", "Unknown XGenericEventCookie\n");
      }
      XFreeEventData(display, cookie);
    } else {
      DEBUG("event", "Unknown GenericEvent without EventData\n");
    }
  } else if (event->type == damage_event + XDamageNotify) {
    XErrorEvent error;
    DEBUG("event.damage", "Received XDamageNotify: %d\n", ((XDamageNotifyEvent *) event)->drawable);
    // Subtract all the damage, repairing the window.
    draw();
    x_try();
    XDamageSubtract(display, ((XDamageNotifyEvent *) event)->damage, None, None);
    x_catch(&error);
  } else if (event->type == shape_event + ShapeNotify) {
   //fprintf(stderr, "Received ShapeNotify\n");
   //XShapeEvent *event = (XShapeEvent*) &e;
  } else if (event->type == ConfigureRequest) {
    Item *item = item_get_from_window(event->xconfigurerequest.window, False);
    if (!item) {
      XWindowChanges values;
      values.width = event->xconfigurerequest.width;
      values.height = event->xconfigurerequest.height;
      XConfigureWindow(display, event->xconfigurerequest.window, CWWidth | CWHeight, &values);
    } else {
      if (item->prop_size) {
        unsigned long width = item->prop_size->values.dwords[0];
        unsigned long height = item->prop_size->values.dwords[1];
        float *coords = (float *) item->prop_coords->data;

        coords[2] *= (float) event->xconfigurerequest.width / (float) width;
        coords[3] *= (float) event->xconfigurerequest.height / (float) height;

        long coords_arr[4];
        for (int i = 0; i < 4; i++) {
          coords_arr[i] = *(long *) &coords[i];
        }
        XChangeProperty(display, item->window, IG_COORDS, XA_FLOAT, 32, PropModeReplace, (void *) coords_arr, 4);

        long arr[2] = {width, height};
        XChangeProperty(display, item->window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);

        item->type->update((Item *) item);
        gl_check_error("item_update_pixmap");
        draw();
      } else {
        DEBUG("error", "%ld: prop_size not set before ConfigureRequest\n", event->xconfigurerequest.window);
      }
    }
  } else if (event->type == ConfigureNotify) {
    DEBUG("event.configure", "Received ConfigureNotify for %ld\n", event->xconfigure.window);
    Item *item = item_get_from_window(event->xconfigure.window, False);
    if (item && item->layer == IG_LAYER_MENU) {
      float coords[4];
      View *v = NULL;
      if (views) {
        v = view_find(views, item->layer);
      }
      if (v) {
        coords[0] = v->screen[0] + (v->screen[2] * (float) event->xconfigure.x) / (float) v->width;
        coords[1] = v->screen[1] + v->screen[3] - (v->screen[3] * (float) event->xconfigure.y) / (float) v->height;
        coords[2] = (v->screen[2] * (float) event->xconfigure.width) / (float) v->width;
        coords[3] = (v->screen[3] * (float) event->xconfigure.height) / (float) v->height;
      } else {
        coords[0] = ((float) (event->xconfigure.x - overlay_attr.x)) / (float) overlay_attr.width;
        coords[1] = ((float) (overlay_attr.height - event->xconfigure.y - overlay_attr.y)) / (float) overlay_attr.width;
        coords[2] = ((float) (event->xconfigure.width)) / (float) overlay_attr.width;
        coords[3] = ((float) (event->xconfigure.height)) / (float) overlay_attr.width;
      }

      float *old_coords = (float *) item->prop_coords->data;
      DEBUG("menu.reconfigure", "%ld: %d,%d->%d,%d[%d,%d]   %f,%f,%f,%f->%f,%f,%f,%f\n",
            item->window,
            item->x, item->y, event->xconfigure.x, event->xconfigure.y, event->xconfigure.width, event->xconfigure.height,
            old_coords[0],old_coords[1],old_coords[2],old_coords[3],
            coords[0],coords[1],coords[2],coords[3]);

      item->x = event->xconfigure.x;
      item->y = event->xconfigure.y;        

      long coords_arr[4];
      for (int i = 0; i < 4; i++) {
        coords_arr[i] = *(long *) &coords[i];
      }
      XChangeProperty(display, item->window, IG_COORDS, XA_FLOAT, 32, PropModeReplace, (void *) coords_arr, 4);

      long arr[2] = {event->xconfigure.width, event->xconfigure.height};
      XChangeProperty(display, item->window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
      item->type->update((Item *) item);
      draw();
    }
    // FIXME: Update width/height regardless of window type...
  } else if (event->type == DestroyNotify) {
    Item * item = item_get_from_window(event->xdestroywindow.window, False);
    if (item) {
      item_remove(item);
    }
  } else if (event->type == ReparentNotify) {
    Item * item = item_get_from_window(event->xreparent.window, False);
    if (item) {
      if (event->xreparent.parent == root) {
        item->type->update(item);
      } else {
        item_remove(item);
      }
      draw();
    }
  } else if (event->type == MapNotify) {
    if (event->xmap.window != overlay) {
      DEBUG("event.map", "MapNotify %ld\n", event->xmap.window);
      Item *item = item_get_from_window(event->xmap.window, True);
      item->is_mapped = True;
      item->type->update(item);
      draw();

      char *window_name;
      if (XFetchName(display, event->xmap.window, &window_name) && window_name) {
        EVENTLOG("window", "{\"window\": %ld, \"name\": \"%s\"}\n", event->xmap.window, window_name);
        XFree(window_name);
      }        
    }
 } else if (event->type == UnmapNotify) {
    Item *item = item_get_from_window(event->xunmap.window, False);
    if (item) {
      item->is_mapped = False;
      draw();
    }
  } else if (event->type == MapRequest) {
    XMapWindow(display, event->xmaprequest.window);
  } else if (event->type == ClientMessage && event->xclient.message_type == IG_DEBUG) {
    printf("DEBUG LIST VIEWS\n");
    for (size_t idx = 0; idx < views->count; idx++) {
      View *view = (View *) views->entries[idx];
      view_print(view);
    }

    printf("DEBUG LIST VIEWS END\n");
    printf("DEBUG LIST ITEMS\n");
    for (size_t idx = 0; idx < items_all->count; idx++) {
      Item *item = (Item *) items_all->entries[idx];
      item->type->print(item);
    }
    printf("DEBUG LIST ITEMS END\n");
  } else if (event->type == ClientMessage && event->xclient.message_type == IG_EXIT) {
    DEBUG("exit", "Exiting by request");
    exit(1);
  } else {
    return False;
  }

  if (EVENTLOG_ENABLED("processing_time")) {
    EVENTLOG("processing_time", "{\"processing_time\": %lu, ", get_timestamp() - start_time);
    print_xevent_fragment(eventlog, display, event);
    EVENTLOG("processing_time", "}\n");
  }

  return True;
}

int main() {
  trace_state = backtrace_create_state(NULL, 0, NULL, NULL);
// backtrace_print (trace_state, 0, stdout);
  
  if (!xinit()) return 1;
  if (!glinit(overlay)) return 1;
  if (!init_picking()) return 1;

  manager_selection_create(XInternAtom(display, "WM_S0", False),
                           &selection_sn_handler,
                           &selection_sn_clear,
                           NULL, True, 0, 0);
  
  DEBUG("start", "Initialized X and GL.\n");

  while (!(views = view_load_all())) sleep(1);
  while (!(shaders = shader_load_all())) sleep(1);

  DEBUG("start", "Initialized views and shaders.\n");

  property_type_register(&property_atom);
  property_type_register(&property_window);
  property_type_register(&property_int);
  property_type_register(&property_float);
  property_type_register(&property_svg);
  property_type_register(&property_wm_hints_icon);
  property_type_register(&property_net_wm_icon);

  items_get_from_toplevel_windows();
 
  gl_check_error("start1");

  draw();

  gl_check_error("start2");

  DEBUG("start", "Renderer started.\n");

  EventHandler main_event_handler;
  main_event_handler.event_mask = SubstructureRedirectMask | SubstructureNotifyMask | PropertyChangeMask;
  event_mask_unset(main_event_handler.match_event);
  event_mask_unset(main_event_handler.match_mask);
  main_event_handler.handler = &main_event_handler_function;
  main_event_handler.data = NULL;
  event_handler_install(&main_event_handler);

  event_mainloop();
  
  return 0;
}
