// Only for gmon out dlsym stuff
#define _GNU_SOURCE 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>
#include <dlfcn.h>

#include "xapi.h"
#include "view.h"
#include "xevent.h"
#include "wm.h"
#include "mainloop.h"
#include "selection.h"
#include "list.h"
#include "debug.h"
#include "fps.h"
#include "item.h"
#include "property.h"
#include "property_atom.h"
#include "property_window.h"
#include "property_int.h"
#include "property_float.h"
#include "property_svg.h"
#include "property_wm_hints_icon.h"
#include "property_net_wm_icon.h"
#include "property_item.h"
#include <X11/extensions/XInput2.h>
#include <math.h>



#define AUTOMATIC_REDRAWS 10

List *views = NULL;
List *shaders = NULL;
GLuint picking_fb;

Atom IG_DEBUG;
Atom IG_EXIT;

Atom current_layer;
Bool filter_by_layer(Item *item) {
  return item->prop_layer && (Atom) item->prop_layer->values.dwords[0] == current_layer;
}

void draw() {
  draw_fps_start();
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0., 0., 0., 1.0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  if (views) {
    for (size_t idx = 0; idx < views->count; idx++) {
      View *v = (View *) views->entries[idx];
      current_layer = v->layer;
      view_draw(0, v, items_all, &filter_by_layer);
    }
  }
  glFlush();
  glXSwapBuffers(display, overlay);
  draw_fps();
}

Bool drawn_this_cycle = False;
int draw_cycles_left = 0;

void cycle_draw() {
  if (draw_cycles_left == 0) {
    drawn_this_cycle = False;
    return;
  }
  draw();
  drawn_this_cycle = True;
  draw_cycles_left--;
}

void trigger_draw() {
  draw_cycles_left = AUTOMATIC_REDRAWS;
  if (!drawn_this_cycle) {
    draw();
    drawn_this_cycle = True;
  }
}

void pick(int x, int y, int *winx, int *winy, Item **item) {
  if (!views) {
    *winy = *winx = 0;
    *item = NULL;
    return;
  }
  View *view = (View *) views->entries[0];
  GL_CHECK_ERROR("pick1", "");
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
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  switch(status) {
    case GL_FRAMEBUFFER_COMPLETE:
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      ERROR("init_picking", "Not all framebuffer attachment points are framebuffer attachment complete.\n");
      return 0;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      ERROR("init_picking", "No images are attached to the framebuffer.\n");
      return 0;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      ERROR("init_picking", "The combination of internal formats of the attached images violates an implementation-dependent set of restrictions.\n");
      return 0;
    default:
      ERROR("init_picking", "Unknown error.\n");
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

  GL_CHECK_ERROR("loop", "");

  if (event->type == PropertyNotify) {
    Bool changed = True;
    Item *item = (Item *) item_get_from_window(event->xproperty.window, False);

    if (item) {
      if (!properties_update(item->properties, event->xproperty.atom)) {
        changed = False;
      }
      if (event->xproperty.atom == IG_LAYER && !item->prop_layer) item->prop_layer = properties_find(item->properties, IG_LAYER);
      if (event->xproperty.atom == IG_SHADER && !item->prop_shader) item->prop_shader = properties_find(item->properties, IG_SHADER);
      if (event->xproperty.atom == IG_SIZE && !item->prop_size) item->prop_size = properties_find(item->properties, IG_SIZE);
      if (event->xproperty.atom == IG_COORDS && !item->prop_coords) item->prop_coords = properties_find(item->properties, IG_COORDS);        
      if (event->xproperty.atom == IG_DRAW_TYPE && !item->prop_draw_type) item->prop_draw_type = properties_find(item->properties, IG_DRAW_TYPE);        
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
          // Do not allow way to big windows, as that screws up OpenGL and X11 and everything will crash...
          if (values.width < 0 || values.height < 0 || values.width > overlay_attr.width * 5 || values.height > overlay_attr.height * 5) {
            long arr[2];
            XWindowAttributes attr;
            XGetWindowAttributes(display, event->xproperty.window, &attr);
            arr[0] = attr.width;
            arr[1] = attr.height;
            XChangeProperty(display, event->xproperty.window, IG_SIZE, XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
          } else {
            XConfigureWindow(display, item->window, CWWidth | CWHeight, &values);
            DEBUG("event.size", "SIZE CHANGED TO %i,%i\n", values.width, values.height);
            item_update((Item *) item);
          }
        }
        XFree(prop_return);
      } else if (event->xproperty.window == root && event->xproperty.atom == IG_VIEWS) {
        view_free_all(views);
        views = view_load_all();
      } else if (event->xproperty.window == root && event->xproperty.atom == IG_SHADERS) {
        shader_free_all(shaders);
        shaders = shader_load_all();       
      } else if (event->xproperty.window == root) {
        Bool handled = False;
        if (views) {
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
        }
        if (!handled) {
          if (DEBUG_ENABLED("event.other")) {
            DEBUG("event.other", "Ignored property event ");
            print_xevent(stderr, display, event);
          }
        }
      }
      trigger_draw();
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
        if (item && (!item->prop_layer || (Atom) item->prop_layer->values.dwords[0] != IG_LAYER_MENU)) {
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
          DEBUG("position", "Point %d,%d -> NONE\n", root_x, root_y);
        }
      } else {
        DEBUG("event", "Unknown XGenericEventCookie\n");
      }
      XFreeEventData(display, cookie);
    } else {
      DEBUG("event", "Unknown GenericEvent without EventData\n");
    }
  } else if (event->type == damage_event + XDamageNotify) {
    DEBUG("event.damage", "Received XDamageNotify: %d\n", ((XDamageNotifyEvent *) event)->drawable);
    // Subtract all the damage, repairing the window.
    Item *item = item_get_from_window(((XDamageNotifyEvent *) event)->drawable, False);
    if (item) {
      item->draw_cycles_left = AUTOMATIC_REDRAWS;
      trigger_draw();
    }
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

        item_update((Item *) item);
        GL_CHECK_ERROR("item_update_pixmap", "%ld", item->window);
        trigger_draw();
      } else {
        DEBUG("error", "%ld: prop_size not set before ConfigureRequest\n", event->xconfigurerequest.window);
      }
    }
  } else if (event->type == ConfigureNotify) {
    DEBUG("event.configure", "Received ConfigureNotify for %ld\n", event->xconfigure.window);
    Item *item = item_get_from_window(event->xconfigure.window, False);
    if (item && item->prop_layer && (Atom) item->prop_layer->values.dwords[0] == IG_LAYER_MENU) {
      float coords[4];
      View *v = NULL;
      if (views) {
        v = view_find(views, (Atom) item->prop_layer->values.dwords[0]);
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

      float old_coords_nan[4] = {nanf("initial"), nanf("initial"), nanf("initial"), nanf("initial")};
      float *old_coords = old_coords_nan;
      if (item->prop_coords) {
        old_coords = (float *) item->prop_coords->data;
      }
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
      item_update((Item *) item);
      trigger_draw();
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
        item_update(item);
      } else {
        item_remove(item);
      }
      trigger_draw();
    }
  } else if (event->type == MapNotify) {
    if (event->xmap.window != overlay) {
      DEBUG("event.map", "MapNotify %ld\n", event->xmap.window);
      Item *item = item_get_from_window(event->xmap.window, True);
      item->is_mapped = True;
      item_update(item);
      trigger_draw();

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
      trigger_draw();
    }
  } else if (event->type == MapRequest) {
    XMapWindow(display, event->xmaprequest.window);
  } else if (event->type == ClientMessage && event->xclient.message_type == IG_DEBUG) {
    printf("DEBUG LIST VIEWS\n");
    if (views) {
      for (size_t idx = 0; idx < views->count; idx++) {
        View *view = (View *) views->entries[idx];
        view_print(view);
      }
    }
    
    printf("DEBUG LIST VIEWS END\n");
    printf("DEBUG LIST ITEMS\n");
    for (size_t idx = 0; idx < items_all->count; idx++) {
      Item *item = (Item *) items_all->entries[idx];
      item_print(item);
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

void draw_timeout_handler_function(TimeoutHandler *handler, struct timeval *current_time) {
  cycle_draw();
}


void exit_saving_profile_info(int sig) {
  fprintf(stderr, "Exiting on SIGUSR1\n");
  void (*_mcleanup)(void);
  _mcleanup = (void (*)(void)) dlsym(RTLD_DEFAULT, "_mcleanup");
  if (_mcleanup == NULL)
    fprintf(stderr, "Unable to find gprof exit hook\n");
  else _mcleanup();
  _exit(0);
}

int main() {
  signal(SIGUSR1, exit_saving_profile_info);
  
  if (!xinit()) return 1;
  if (!init_view()) return 1;
  if (!init_selection()) return 1;
  if (!glinit(overlay)) return 1;
  if (!init_picking()) return 1;
  if (!init_shader()) return 1;
  if (!init_items()) return 1;

  IG_DEBUG = XInternAtom(display, "IG_DEBUG", False);
  IG_EXIT = XInternAtom(display, "IG_EXIT", False);
  
  manager_selection_create(XInternAtom(display, "WM_S0", False),
                           &selection_sn_handler,
                           &selection_sn_clear,
                           NULL, True, 0, 0);
  
  DEBUG("start", "Initialized X and GL.\n");

  views = view_load_all();
  shaders = shader_load_all();

  DEBUG("XXXXXXXXXXX1", "views=%ld shaders=%ld\n", views, shaders);
  
  property_type_register(&property_atom);
  property_type_register(&property_window);
  property_type_register(&property_int);
  property_type_register(&property_float);
  property_type_register(&property_svg);
  property_type_register(&property_wm_hints_icon);
  property_type_register(&property_net_wm_icon);
  property_type_register(&property_item);

  items_get_from_toplevel_windows();
 
  GL_CHECK_ERROR("start1", "");

  trigger_draw();

  GL_CHECK_ERROR("start2", "");

  DEBUG("start", "Renderer started.\n");

  TimeoutHandler draw_timeout_handler;
  draw_timeout_handler.interval.tv_sec = 0;
  draw_timeout_handler.interval.tv_usec = 30000;
  if (gettimeofday(&draw_timeout_handler.next, NULL) != 0) {
    ERROR("gettimeofday", "gettimeofday() returned error");
  }
  draw_timeout_handler.handler = &draw_timeout_handler_function;
  draw_timeout_handler.data = NULL;
  mainloop_install_timeout_handler(&draw_timeout_handler);
  
  EventHandler main_event_handler;
  main_event_handler.event_mask = SubstructureRedirectMask | SubstructureNotifyMask | PropertyChangeMask;
  event_mask_unset(main_event_handler.match_event);
  event_mask_unset(main_event_handler.match_mask);
  main_event_handler.handler = &main_event_handler_function;
  main_event_handler.data = NULL;
  mainloop_install_event_handler(&main_event_handler);

  mainloop_run();
  
  return 0;
}
