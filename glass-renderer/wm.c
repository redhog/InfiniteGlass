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
#include "item_widget.h"
#include "event.h"
#include "selection.h"
#include "list.h"
#include <X11/extensions/XInput2.h>
#include <SOIL/SOIL.h>

static Bool debug_positions = False;

Window motion_notification_window;
List *views;
GLuint picking_fb;

Atom current_layer;
Bool filter_by_layer(Item *item) {
  return item->layer == current_layer;
}

void draw() {
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
  
  fprintf(stderr, "Initialized X and GL.\n");

  while (!(views = view_load_all())) sleep(1);

  motion_notification_window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);
  
  XChangeProperty(display, root, IG_NOTIFY_MOTION, XA_WINDOW, 32, PropModeReplace, (void *) &motion_notification_window, 1);
  
  items_get_from_toplevel_windows();
 
  gl_check_error("start1");

  draw();

  gl_check_error("start2");

  for (;;) {
    XEvent e;
    XGenericEventCookie *cookie = &e.xcookie;
    XSync(display, False);
    XNextEvent(display, &e);
//    if (e.type != MotionNotify && e.type != GenericEvent && e.type != damage_event + XDamageNotify) {
//      print_xevent(display, &e);
//    }
    
    gl_check_error("loop");

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
          if (item && item->layer != IG_LAYER_MENU && item_isinstance(item, &item_type_window)) {
            ItemWindow *window_item = (ItemWindow *) item;
            win = window_item->window;

            XWindowChanges values;
            values.x = root_x - winx;
            values.y = root_y - winy;
            values.stack_mode = Above;
            if (values.x != window_item->x || values.y != window_item->y) {
              XConfigureWindow(display, window_item->window, CWX | CWY | CWStackMode, &values);
              window_item->x = values.x;
              window_item->y = values.y;
            }

            if (debug_positions) {
              printf("Point %d,%d -> %lu,%d,%d\n", e.xmotion.x_root, e.xmotion.y_root, window_item->window, winx, winy); fflush(stdout);
            }
          } else {
            if (debug_positions) {
              printf("Point %d,%d -> NONE\n", e.xmotion.x_root, e.xmotion.y_root); fflush(stdout);
            }
          }

          long coords[views->count * 2];
          for (int i=0; i < views->count; i++) {
            view_to_space((View *) views->entries[i],
                          root_x, root_y - ((View *) views->entries[i])->height,
                          (float *) &coords[i*2], (float *) &coords[i*2+1]);
          }
          XChangeProperty(display, motion_notification_window, IG_NOTIFY_MOTION, XA_FLOAT, 32, PropModeReplace, (void *) coords, 2*views->count);
          XChangeProperty(display, motion_notification_window, IG_ACTIVE_WINDOW, XA_WINDOW, 32, PropModeReplace, (void *) &win, 1);
        } else {
          printf("Unknown XGenericEventCookie\n");
        }
        XFreeEventData(display, cookie);
      } else {
        printf("Unknown GenericEvent without EventData\n");
      }
    } else if (event_handle(&e)) {
     // Already handled
    } else if (e.type == damage_event + XDamageNotify) {
      XErrorEvent error;
      XDamageNotifyEvent *event = (XDamageNotifyEvent*) &e;
      //fprintf(stderr, "Received XDamageNotify: %d\n", event->drawable);

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
      ItemWindow *item = (ItemWindow *) item_get_from_window(event->window, False);
      if (!item) {
        XWindowChanges values;
        values.width = event->width;
        values.height = event->height;
        XConfigureWindow(display, event->window, CWWidth | CWHeight, &values);
      } else {
        item->base.coords[2] *= (float) event->width / (float) item->base.width;
        item->base.coords[3] *= (float) event->height / (float) item->base.height;
        item->base.width = event->width;
        item->base.height = event->height;
        item->width_window = event->width;
        item->height_window = event->height;
        item->base.type->update((Item *) item);
        gl_check_error("item_update_pixmap");
        draw();
      }
    } else if (e.type == ConfigureNotify) {
      fprintf(stderr, "Received ConfigureNotify for %ld\n", e.xconfigure.window);
      XConfigureEvent *event = (XConfigureEvent*) &e;
      ItemWindow *item = (ItemWindow *) item_get_from_window(event->window, False);
      if (!item) continue;
      if (item->base.layer == IG_LAYER_MENU) {       
        item->base.coords[0] = ((float) (event->x - overlay_attr.x)) / (float) overlay_attr.width;
        item->base.coords[1] = ((float) (overlay_attr.height - event->y - overlay_attr.y)) / (float) overlay_attr.width;
        item->base.coords[2] = ((float) (event->width)) / (float) overlay_attr.width;
        item->base.coords[3] = ((float) (event->height)) / (float) overlay_attr.width;
        item->base.width = event->width;
        item->base.height = event->height;
        item->width_window = event->width;
        item->height_window = event->height;
        item->base.type->update((Item *) item);
        draw();
      }
      // FIXME: Update width/height regardless of window type...
    } else if (e.type == PropertyNotify && e.xproperty.atom == IG_SIZE) {
      Atom type_return;
      int format_return;
      unsigned long nitems_return;
      unsigned long bytes_after_return;
      unsigned char *prop_return;
      ItemWindow *item = (ItemWindow *) item_get_from_window(e.xproperty.window, False);
      if (!item) continue;
      XGetWindowProperty(display, e.xproperty.window, IG_SIZE, 0, sizeof(long)*2, 0, AnyPropertyType,
                         &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
      if (type_return != None) {
        item->base.width = ((long *) prop_return)[0];
        item->base.height = ((long *) prop_return)[1];
        item->width_property = item->base.width;
        item->height_property = item->base.height;
        //printf("SIZE CHANGED TO %i,%i\n", item->width, item->height);
        item->base.type->update((Item *) item);
        draw();
      }
      XFree(prop_return);
    } else if (e.type == PropertyNotify && e.xproperty.atom == DISPLAYSVG) {
      Item * item = item_get_from_window(e.xproperty.window, False);
      if (!item) continue;
      // FIXME: Handle updates of existign item later
      // if (!item_isinstance(item, &item_type_window_svg)) {
      item_remove(item);
      item = item_get_from_window(e.xproperty.window, True);
/*
     } else if (e.type == CreateNotify) {
      if (e.xcreatewindow.window != overlay && e.xcreatewindow.window != motion_notification_window) {
        fprintf(stderr, "CreateNotify %ld under %ld @ %d,%d size = %d, %d\n", e.xcreatewindow.window, e.xcreatewindow.parent, e.xcreatewindow.x, e.xcreatewindow.y, e.xcreatewindow.width, e.xcreatewindow.height);
        Item *item = item_get_from_window(e.xcreatewindow.window, False);
        if (!item) continue;
        // XMapWindow(display, e.xmaprequest.window);
        item->type->update(item);
      }
*/
    } else if (e.type == DestroyNotify) {
      Item * item = item_get_from_window(e.xdestroywindow.window, False);
      if (!item) continue;
      item_remove(item);
    } else if (e.type == ReparentNotify) {
      if (e.xreparent.parent == root) {
        Item * item = item_get_from_window(e.xreparent.window, False);
        if (!item) continue;
        item->type->update(item);
      } else {
        Item * item = item_get_from_window(e.xreparent.window, False);
        if (!item) continue;
        item_remove(item);
      }
      draw();
    } else if (e.type == MapNotify) {
      if (e.xmap.window != overlay) {
        fprintf(stderr, "MapNotify %ld\n", e.xmap.window);
        Item *item = item_get_from_window(e.xmap.window, True);
        item->is_mapped = True;
        item->type->update(item);
        draw();
      }
   } else if (e.type == UnmapNotify) {
      Item *item = item_get_from_window(e.xunmap.window, False);
      if (!item) continue;
      item->is_mapped = False;
      draw();
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
        if (!item) continue;
        item_type_window_update_space_pos_from_window((ItemWindow *) item);
        item->type->update(item);
        draw();
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
        print_xevent(display, &e);
        fprintf(stderr, "Ignored event\n"); fflush(stderr);
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
      printf("Exiting by request");
      exit(1);
    } else {
//      print_xevent(display, &e);
//      fprintf(stderr, "Ignored event\n"); fflush(stderr);
    }
  }
  return 0;
}
