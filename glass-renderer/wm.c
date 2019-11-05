#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

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
#include <X11/extensions/XInput2.h>
#include <SOIL/SOIL.h>

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
  glClearColor(1.0, 1.0, 0.5, 1.0);
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


int main() {
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
  
  property_type_register(&property_int);
  property_type_register(&property_float);
  
  items_get_from_toplevel_windows();
 
  gl_check_error("start1");

  draw();

  gl_check_error("start2");

  DEBUG("start", "Renderer started.\n");
  
  for (;;) {
    XEvent e;
    XGenericEventCookie *cookie = &e.xcookie;
    XSync(display, False);
    XNextEvent(display, &e);

    unsigned long start_time = get_timestamp();
    
    gl_check_error("loop");

    if (e.type == PropertyNotify) {
      Item *item = (Item *) item_get_from_window(e.xproperty.window, False);
      if (item) {
        properties_update(item->properties, item->window, e.xproperty.atom);
        draw();
      }
   }
    
    if (cookie->type == GenericEvent) {
      if (XGetEventData(display, cookie)) {
        if (cookie->evtype == XI_RawMotion) {
          //XIRawEvent *re = (XIRawEvent *) cookie->data;
          Window       root_ret, child_ret;
          int          root_x, root_y;
          int          win_x, win_y;
          unsigned int mask;
          XQueryPointer(display, root,
                        &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);

          int winx, winy;
          Item *item;
          Window win = root;
          pick(root_x, root_y, &winx, &winy, &item);
          if (item && item->layer != IG_LAYER_MENU && item_isinstance(item, &item_type_base)) {
            win = item->window;

            XWindowChanges values;
            values.x = root_x - winx;
            values.y = root_y - winy;
            values.stack_mode = Above;
            if (values.x != item->x || values.y != item->y) {
              XConfigureWindow(display, item->window, CWX | CWY | CWStackMode, &values);
              item->x = values.x;
              item->y = values.y;
            }

            DEBUG("position", "Point %d,%d -> %lu,%d,%d\n", e.xmotion.x_root, e.xmotion.y_root, item->window, winx, winy);
          } else {
            DEBUG("position", "Point %d,%d -> NONE\n", e.xmotion.x_root, e.xmotion.y_root);
          }
        } else {
          DEBUG("event", "Unknown XGenericEventCookie\n");
        }
        XFreeEventData(display, cookie);
      } else {
        DEBUG("event", "Unknown GenericEvent without EventData\n");
      }
    } else if (event_handle(&e)) {
     // Already handled
    } else if (e.type == damage_event + XDamageNotify) {
      XErrorEvent error;
      XDamageNotifyEvent *event = (XDamageNotifyEvent*) &e;
      DEBUG("event.damage", "Received XDamageNotify: %d\n", event->drawable);

      // Subtract all the damage, repairing the window.
      draw();
      x_try();
      XDamageSubtract(display, event->damage, None, None);
      x_catch(&error);
    } else if (e.type == shape_event + ShapeNotify) {
     //fprintf(stderr, "Received ShapeNotify\n");
     //XShapeEvent *event = (XShapeEvent*) &e;
    } else if (e.type == ConfigureRequest) {
      XConfigureRequestEvent *event = (XConfigureRequestEvent*) &e;
      Item *item = item_get_from_window(event->window, False);
      if (!item) {
        XWindowChanges values;
        values.width = event->width;
        values.height = event->height;
        XConfigureWindow(display, event->window, CWWidth | CWHeight, &values);
      } else {
        item->coords[2] *= (float) event->width / (float) item->width;
        item->coords[3] *= (float) event->height / (float) item->height;
        item->width = event->width;
        item->height = event->height;
        item->width_window = event->width;
        item->height_window = event->height;
        item->type->update((Item *) item);
        gl_check_error("item_update_pixmap");
        draw();
      }
    } else if (e.type == ConfigureNotify) {
      DEBUG("event.configure", "Received ConfigureNotify for %ld\n", e.xconfigure.window);
      XConfigureEvent *event = (XConfigureEvent*) &e;
      Item *item = item_get_from_window(event->window, False);
      if (item && item->layer == IG_LAYER_MENU) {       
        item->coords[0] = ((float) (event->x - overlay_attr.x)) / (float) overlay_attr.width;
        item->coords[1] = ((float) (overlay_attr.height - event->y - overlay_attr.y)) / (float) overlay_attr.height;
        item->coords[2] = ((float) (event->width)) / (float) overlay_attr.width;
        item->coords[3] = ((float) (event->height)) / (float) overlay_attr.height;
        item->width = event->width;
        item->height = event->height;
        item->width_window = event->width;
        item->height_window = event->height;
        item->type->update((Item *) item);
        draw();
      }
      // FIXME: Update width/height regardless of window type...
    } else if (e.type == PropertyNotify && e.xproperty.atom == IG_SIZE) {
      Atom type_return;
      int format_return;
      unsigned long nitems_return;
      unsigned long bytes_after_return;
      unsigned char *prop_return;
      Item *item = item_get_from_window(e.xproperty.window, False);
      if (item) {
        XGetWindowProperty(display, e.xproperty.window, IG_SIZE, 0, sizeof(long)*2, 0, AnyPropertyType,
                           &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
        if (type_return != None) {
          item->width = ((long *) prop_return)[0];
          item->height = ((long *) prop_return)[1];
          item->width_property = item->width;
          item->height_property = item->height;
          DEBUG("event.size", "SIZE CHANGED TO %i,%i\n", item->width, item->height);
          item->type->update((Item *) item);
          draw();
        }
        XFree(prop_return);
      }
    } else if (e.type == PropertyNotify && e.xproperty.atom == DISPLAYSVG) {
      Item * item = item_get_from_window(e.xproperty.window, False);
      if (item) {
        // FIXME: Handle updates of existign item later
        // if (!item_isinstance(item, &item_type_window_svg)) {
        item_remove(item);
        item = item_get_from_window(e.xproperty.window, True);
      }
    } else if (e.type == DestroyNotify) {
      Item * item = item_get_from_window(e.xdestroywindow.window, False);
      if (item) {
        item_remove(item);
      }
    } else if (e.type == ReparentNotify) {
      Item * item = item_get_from_window(e.xreparent.window, False);
      if (item) {
        if (e.xreparent.parent == root) {
          item->type->update(item);
        } else {
          item_remove(item);
        }
        draw();
      }
    } else if (e.type == MapNotify) {
      if (e.xmap.window != overlay) {
        DEBUG("event.map", "MapNotify %ld\n", e.xmap.window);
        Item *item = item_get_from_window(e.xmap.window, True);
        item->is_mapped = True;
        item->type->update(item);
        draw();

        char *window_name;
        if (XFetchName(display, e.xmap.window, &window_name) && window_name) {
          EVENTLOG("window", "{\"window\": %ld, \"name\": \"%s\"}\n", e.xmap.window, window_name);
          XFree(window_name);
        }        
      }
   } else if (e.type == UnmapNotify) {
      Item *item = item_get_from_window(e.xunmap.window, False);
      if (item) {
        item->is_mapped = False;
        draw();
      }
    } else if (e.type == MapRequest) {
      XMapWindow(display, e.xmaprequest.window);
    } else if (e.type == PropertyNotify) {
      Bool handled = False;
      if (   (e.xproperty.window == root)
          && (e.xproperty.atom == IG_VIEWS)) {
        view_free_all(views);
        views = view_load_all();
        draw();
        handled=True;
      } else if (e.xproperty.atom == IG_COORDS) {
        Item *item = item_get_from_window(e.xproperty.window, False);
        if (item) {
          item_type_window_update_space_pos_from_window(item);
          item->type->update(item);
          draw();
        }
        handled=True;
      } else {
        for (size_t idx = 0; idx < views->count; idx++) {
          View *v = (View *) views->entries[idx];
          if (e.xproperty.atom == v->attr_layer) {
            view_load_layer(v);
            draw();
            handled=True;
          } else if (e.xproperty.atom == v->attr_view) {
            view_load_screen(v);
            draw();
            handled=True;
          }
        }
      }
      if (!handled) {
        if (DEBUG_ENABLED("event.other")) {
          DEBUG("event.other", "Ignored event ");
          print_xevent(stderr, display, &e);
        }
      }
    } else if (e.type == ClientMessage && e.xclient.message_type == IG_DEBUG) {
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
    } else if (e.type == ClientMessage && e.xclient.message_type == IG_EXIT) {
      DEBUG("exit", "Exiting by request");
      exit(1);
    } else {
      if (DEBUG_ENABLED("event.other")) {
        DEBUG("event.other", "Ignored event ");
        print_xevent(stderr, display, &e);
      }
    }

    if (EVENTLOG_ENABLED("processing_time")) {
      EVENTLOG("processing_time", "{\"processing_time\": %lu, ", get_timestamp() - start_time);
      print_xevent_fragment(eventlog, display, &e);
      EVENTLOG("processing_time", "}\n");
    }
  }
  return 0;
}
