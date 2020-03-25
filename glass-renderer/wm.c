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
#include "property_coords.h"
#include "property_svg.h"
#include "property_wm_hints_icon.h"
#include "property_net_wm_icon.h"
#include "property_item.h"
#include "picking.h"
#include "rendering.h"
#include <X11/extensions/XInput2.h>
#include <math.h>


Pointer mouse = {0, 0, 0, 0, 0, 0};
Mainloop *mainloop = NULL;
XConnection *xconn = NULL;

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
    Item *item = (Item *) item_get_from_window(xconn, event->xproperty.window, False);

    if (item && !item_properties_update(xconn, item, event->xproperty.atom)) {
      changed = False;
    }
    
    if (changed) {
      // FIXME: Test that item != NULL here...
      if (event->xproperty.window != xconn->root && item && event->xproperty.atom == ATOM(xconn, "IG_SIZE")) {
        Atom type_return;
        int format_return;
        unsigned long nitems_return;
        unsigned long bytes_after_return;
        unsigned char *prop_return;
        XGetWindowProperty(xconn->display, event->xproperty.window, ATOM(xconn, "IG_SIZE"), 0, sizeof(long)*2, 0, AnyPropertyType,
                           &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
        if (type_return != None) {
          XWindowChanges values;
          values.width = ((long *) prop_return)[0];
          values.height = ((long *) prop_return)[1];
          XWindowAttributes attr;
          XGetWindowAttributes(xconn->display, event->xproperty.window, &attr);
          
          if (attr.width != values.width || attr.height != values.height) {
            // Do not allow way to big windows, as that screws up OpenGL and X11 and everything will crash...
            if (values.width < 0 || values.height < 0 || values.width > xconn->overlay_attr.width * 5 || values.height > xconn->overlay_attr.height * 5) {
              long arr[2];
              arr[0] = attr.width;
              arr[1] = attr.height;
              DEBUG("event.size", "%ld: Warning IG_SIZE outside of bounds, resetting to %i,%i\n", event->xproperty.window, attr.width, attr.height);
              XChangeProperty(xconn->display, event->xproperty.window, ATOM(xconn, "IG_SIZE"), XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
            } else {
              DEBUG("event.size", "%ld: SIZE CHANGED TO %i,%i\n", event->xproperty.window, values.width, values.height);
              XConfigureWindow(xconn->display, event->xproperty.window, CWWidth | CWHeight, &values);
              item_trigger_update((Item *) item);
            }
          }
        }
        XFree(prop_return);
      } else if (event->xproperty.window == xconn->root && event->xproperty.atom == ATOM(xconn, "IG_SHADERS")) {
        shaders_update(xconn);
      } else if (event->xproperty.window == xconn->root) {
       if (!views_update(xconn, event->xproperty.atom)) {
          if (DEBUG_ENABLED("event.other")) {
            DEBUG("event.other", "Ignored property event ");
            print_xevent(stderr, xconn->display, event);
          }
        }
      }
      trigger_draw();
    }
  } else if (cookie->type == GenericEvent) {
    if (XGetEventData(xconn->display, cookie)) {
      if (cookie->evtype == XI_RawMotion) {
        // XIRawEvent *re = (XIRawEvent *) cookie->data;
        XQueryPointer(xconn->display, xconn->root,
                      &mouse.root, &mouse.win, &mouse.root_x, &mouse.root_y, &mouse.win_x, &mouse.win_y, &mouse.mask);

        int winx, winy;
        Item *item;
        Item *parent_item;

        pick(xconn, mouse.root_x, mouse.root_y, &winx, &winy, &item, &parent_item);
        if (item && (!item->prop_layer || !item->prop_layer->values.dwords || (Atom) item->prop_layer->values.dwords[0] != ATOM(xconn, "IG_LAYER_MENU"))) {
          XWindowChanges values;
          values.x = mouse.root_x - winx;
          values.y = mouse.root_y - winy;
          values.stack_mode = Above;
          if (values.x != item->x || values.y != item->y) {
            XConfigureWindow(xconn->display, item->window, CWX | CWY | CWStackMode, &values);
            item->x = values.x;
            item->y = values.y;
          }

          if (parent_item && (parent_item != item->parent_item)) {
            XChangeProperty(xconn->display, item->window, ATOM(xconn, "IG_PARENT_WINDOW"), XA_WINDOW, 32, PropModeReplace, (void *) &parent_item->window, 1);
            item->parent_item = parent_item;
          }
          
          DEBUG("position", "Point %d,%d -> %lu/%lu,%d,%d\n", event->xmotion.x_root, event->xmotion.y_root, parent_item ? parent_item->window : 0, item->window, winx, winy);
        } else {
          DEBUG("position", "Point %d,%d -> NONE\n", mouse.root_x, mouse.root_y);
        }
        trigger_draw();
      } else {
        DEBUG("event", "Unknown XGenericEventCookie\n");
      }
      XFreeEventData(xconn->display, cookie);
    } else {
      DEBUG("event", "Unknown GenericEvent without EventData\n");
    }
  } else if (event->type == xconn->shape_event + ShapeNotify) {
   //fprintf(stderr, "Received ShapeNotify\n");
   //XShapeEvent *event = (XShapeEvent*) &e;
  } else if (event->type == ConfigureRequest) {
    Item *item = item_get_from_window(xconn, event->xconfigurerequest.window, False);
    if (!item) {
      XWindowChanges values;
      values.width = event->xconfigurerequest.width;
      values.height = event->xconfigurerequest.height;
      XConfigureWindow(xconn->display, event->xconfigurerequest.window, CWWidth | CWHeight, &values);
    } else {
      if (item->prop_size) {
        unsigned long width = item->prop_size->values.dwords[0];
        unsigned long height = item->prop_size->values.dwords[1];
        float *orig_coords = ((PropertyCoords *) item->prop_coords->data)->coords;
        float coords[4] = {orig_coords[0], orig_coords[1], orig_coords[2], orig_coords[3]};
        
        coords[2] *= (float) event->xconfigurerequest.width / (float) width;
        coords[3] *= (float) event->xconfigurerequest.height / (float) height;

        DEBUG("configure", "%ld.ConfigureRequest(%d,%d @ %f,%f[%f,%f], %d,%d): %f,%f => %f,%f[%f,%f]\n",
              event->xconfigurerequest.window,
              width,
              height,
              orig_coords[0],
              orig_coords[1],
              orig_coords[2],
              orig_coords[3],
              event->xconfigurerequest.width,
              event->xconfigurerequest.height,
              (float) event->xconfigurerequest.width / (float) width,
              (float) event->xconfigurerequest.height / (float) height,
              coords[0],coords[1],coords[2],coords[3]);
        long coords_arr[4];
        for (int i = 0; i < 4; i++) {
          coords_arr[i] = *(long *) &coords[i];
        }
        XChangeProperty(xconn->display, item->window, ATOM(xconn, "IG_COORDS"), XA_FLOAT, 32, PropModeReplace, (void *) coords_arr, 4);

        long arr[2] = {width, height};
        XChangeProperty(xconn->display, item->window, ATOM(xconn, "IG_SIZE"), XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);

        item_trigger_update((Item *) item);
        GL_CHECK_ERROR("item_trigger_update_pixmap", "%ld", item->window);
        trigger_draw();
      } else {
        DEBUG("error", "%ld: prop_size not set before ConfigureRequest\n", event->xconfigurerequest.window);
      }
    }
  } else if (event->type == ConfigureNotify) {
    DEBUG("event.configure", "%ld.ConfigureNotify(%d,%d[%d,%d])\n",
          event->xconfigure.window,
          event->xconfigure.x,
          event->xconfigure.y,
          event->xconfigure.width,
          event->xconfigure.height);
    Item *item = item_get_from_window(xconn, event->xconfigure.window, False);
    if (item && item->prop_layer && item->prop_layer->values.dwords && (Atom) item->prop_layer->values.dwords[0] == ATOM(xconn, "IG_LAYER_MENU")) {
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
        coords[0] = ((float) (event->xconfigure.x - xconn->overlay_attr.x)) / (float) xconn->overlay_attr.width;
        coords[1] = ((float) (xconn->overlay_attr.height - event->xconfigure.y - xconn->overlay_attr.y)) / (float) xconn->overlay_attr.width;
        coords[2] = ((float) (event->xconfigure.width)) / (float) xconn->overlay_attr.width;
        coords[3] = ((float) (event->xconfigure.height)) / (float) xconn->overlay_attr.width;
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
      XChangeProperty(xconn->display, item->window, ATOM(xconn, "IG_COORDS"), XA_FLOAT, 32, PropModeReplace, (void *) coords_arr, 4);

      long arr[2] = {event->xconfigure.width, event->xconfigure.height};
      XChangeProperty(xconn->display, item->window, ATOM(xconn, "IG_SIZE"), XA_INTEGER, 32, PropModeReplace, (void *) arr, 2);
      item_trigger_update((Item *) item);
      trigger_draw();
    }
    // FIXME: Update width/height regardless of window type...
  } else if (event->type == DestroyNotify) {
    Item * item = item_get_from_window(xconn, event->xdestroywindow.window, False);
    if (item) {
      item_remove(xconn, item);
    }
  } else if (event->type == ReparentNotify) {
    Item * item = item_get_from_window(xconn, event->xreparent.window, False);
    if (item) {
      if (event->xreparent.parent == xconn->root) {
        item_trigger_update(item);
      } else {
        item_remove(xconn, item);
      }
      trigger_draw();
    }
  } else if (event->type == MapNotify) {
    if (event->xmap.window != xconn->overlay) {
      DEBUG("event.map", "MapNotify %ld\n", event->xmap.window);
      Item *item = item_get_from_window(xconn, event->xmap.window, True);
      item->is_mapped = True;
      item_trigger_update(item);
      trigger_draw();

      char *window_name;
      if (XFetchName(xconn->display, event->xmap.window, &window_name) && window_name) {
        EVENTLOG("window", "{\"window\": %ld, \"name\": \"%s\"}\n", event->xmap.window, window_name);
        XFree(window_name);
      }        
    }
 } else if (event->type == UnmapNotify) {
    Item *item = item_get_from_window(xconn, event->xunmap.window, False);
    if (item) {
      item->is_mapped = False;
      trigger_draw();
    }
  } else if (event->type == MapRequest) {
    XMapWindow(xconn->display, event->xmaprequest.window);
  } else if (event->type == ClientMessage && event->xclient.message_type == ATOM(xconn, "IG_DEBUG")) {
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
      item_print(xconn, item);
    }
    printf("DEBUG LIST ITEMS END\n");
  } else if (event->type == ClientMessage && event->xclient.message_type == ATOM(xconn, "IG_EXIT")) {
    DEBUG("exit", "Exiting by request");
    exit(1);
  } else if (event->type == ClientMessage && event->xclient.message_type == ATOM(xconn, "IG_DEBUG_PICKING")) {
    debug_picking = !debug_picking;
    trigger_draw();
  } else {
    return False;
  }

  if (EVENTLOG_ENABLED("processing_time")) {
    EVENTLOG("processing_time", "{\"processing_time\": %lu, ", get_timestamp() - start_time);
    print_xevent_fragment(eventlog, xconn->display, event);
    EVENTLOG("processing_time", "}\n");
  }

  return True;
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
  
  if (!(xconn = xinit())) return 1;
  if (!init_view()) return 1;
  if (!init_selection()) return 1;
  if (!glinit(xconn, xconn->overlay)) return 1;
  if (!init_picking(xconn)) return 1;
  if (!init_shader()) return 1;
  if (!init_items()) return 1;

  mainloop = mainloop_create(xconn);

  manager_selection_create(mainloop,
                           ATOM(xconn, "WM_S0"),
                           &selection_sn_handler,
                           &selection_sn_clear,
                           NULL, True, 0, 0);
  
  DEBUG("start", "Initialized X and GL.\n");

  views_update(xconn, ATOM(xconn, "IG_VIEWS"));
  shaders_update(xconn);

  DEBUG("XXXXXXXXXXX1", "views=%ld shaders=%ld\n", views, shaders);
  
  property_type_register(xconn, &property_atom);
  property_type_register(xconn, &property_window);
  property_type_register(xconn, &property_int);
  property_type_register(xconn, &property_float);
  property_type_register(xconn, &property_coords);
  property_type_register(xconn, &property_svg);
  property_type_register(xconn, &property_wm_hints_icon);
  property_type_register(xconn, &property_net_wm_icon);
  property_type_register(xconn, &property_item);

  items_get_from_toplevel_windows(xconn);
 
  GL_CHECK_ERROR("start1", "");

  init_rendering(mainloop);

  GL_CHECK_ERROR("start2", "");

  DEBUG("start", "Renderer started.\n");
    
  EventHandler main_event_handler;
  main_event_handler.mainloop = mainloop;
  main_event_handler.event_mask = SubstructureRedirectMask | SubstructureNotifyMask | PropertyChangeMask;
  event_mask_unset(main_event_handler.match_event);
  event_mask_unset(main_event_handler.match_mask);
  main_event_handler.handler = &main_event_handler_function;
  main_event_handler.data = NULL;
  mainloop_install_event_handler(&main_event_handler);

  mainloop_run(mainloop);
  
  return 0;
}
