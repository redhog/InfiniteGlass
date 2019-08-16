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

#include <SOIL/SOIL.h>

static Bool debug_positions = False;

Window motion_notification_window;
View **views;

void initItems() {
  XWindowAttributes attr;
  
  XCompositeRedirectSubwindows(display, root, CompositeRedirectAutomatic);

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
    if (top_level_windows[i] == motion_notification_window) continue;
    item_get_from_window(top_level_windows[i]);
  }

  XFree(top_level_windows);
  XUngrabServer(display);
}

Atom current_layer;
Bool filter_by_layer(Item *item) {
  return item->layer == current_layer;
}

void draw() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(1.0, 1.0, 0.5, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  for (View **v = views; *v; v++) {
    current_layer = (*v)->layer;
    view_draw(0, *v, items_all, &filter_by_layer);
  }
  glFlush();
  glXSwapBuffers(display, overlay);
}
GLint picking_fb;

void pick(int x, int y, int *winx, int *winy, Item **item) {
  gl_check_error("pick1");
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  for (View **v = views; *v; v++) {
    current_layer = (*v)->layer;
    view_draw_picking(picking_fb, *v, items_all, &filter_by_layer);
  }
  glFlush();
  view_pick(picking_fb, *views, x, y, winx, winy, item);
}

int init_picking() {
  GLuint color_tex;
  GLint depth_rb;
  
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

int main() {
  if (!xinit()) return 1;
  if (!glinit(overlay)) return 1;
  if (!init_picking()) return 1;

  fprintf(stderr, "Initialized X and GL.\n");

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  while (!(views = view_load_all())) sleep(1);

  motion_notification_window = XCreateSimpleWindow(display, root, 0, 0, 1, 1, 0, 0, 0);
  
  XChangeProperty(display, root, IG_NOTIFY_MOTION, XA_WINDOW, 32, PropModeReplace, &motion_notification_window, 1);
  
  for (View **v = views; *v; v++) {
   printf("VIEW: layer=%s screen=%f,%f,%f,%f\n",
          XGetAtomName(display, v[0]->layer),
          v[0]->screen[0],
          v[0]->screen[1],
          v[0]->screen[2],
          v[0]->screen[3]);
  }
  
  initItems();
 
  glViewport(0, 0, overlay_attr.width, overlay_attr.height);  

  gl_check_error("start1");

  draw();

  gl_check_error("start2");

  for (;;) {
    XEvent e;
    XSync(display, False);
    XNextEvent(display, &e);
    //print_xevent(display, &e);

    gl_check_error("loop");

    if (e.type == damage_event + XDamageNotify) {
      XErrorEvent error;
      XDamageNotifyEvent *event = (XDamageNotifyEvent*) &e;
      //fprintf(stderr, "Received XDamageNotify: %d\n", event->drawable);
      // e->drawable is the window ID of the damaged window
      // e->geometry is the geometry of the damaged window	
      // e->area     is the bounding rect for the damaged area	
      // e->damage   is the damage handle returned by XDamageCreate()

      // Subtract all the damage, repairing the window.
      draw();
      x_try();
      XDamageSubtract(display, event->damage, None, None);
      x_catch(&error);
    } else if (e.type == shape_event + ShapeNotify) {
     //fprintf(stderr, "Received ShapeNotify\n");
      XShapeEvent *event = (XShapeEvent*) &e;
    } else if (e.type == ConfigureRequest) {
      XConfigureRequestEvent *event = (XConfigureRequestEvent*) &e;
      XWindowChanges changes;
      // Copy fields from e to changes.
      changes.x = event->x;
      changes.y = event->y;
      changes.width = event->width;
      changes.height = event->height;
      changes.border_width = event->border_width;
      changes.sibling = event->above;
      changes.stack_mode = event->detail;
      // Grant request by calling XConfigureWindow().
      XConfigureWindow(display, event->window, event->value_mask, &changes);  
      Item *item = item_get_from_window(event->window);
    } else if (e.type == ConfigureNotify) {
      //fprintf(stderr, "Received ConfigureNotify for %ld\n", e.xconfigure.window);
      Item *item = item_get_from_window(e.xconfigure.window);
      gl_check_error("item_update_space_pos_from_window");
      item->type->update(item);
      gl_check_error("item_update_pixmap");
      draw();
    } else if (e.type == CreateNotify) {
      if (e.xcreatewindow.window != overlay && e.xcreatewindow.window != motion_notification_window) {
        fprintf(stderr, "CreateNotify %ld under %ld @ %d,%d size = %d, %d\n", e.xcreatewindow.window, e.xcreatewindow.parent, e.xcreatewindow.x, e.xcreatewindow.y, e.xcreatewindow.width, e.xcreatewindow.height);
        Item *item = item_get_from_window(e.xcreatewindow.window);
        XMapWindow(display, e.xmaprequest.window);
        item->type->update(item);
      }
    } else if (e.type == DestroyNotify) {
      Item * item = item_get_from_window(e.xdestroywindow.window);
      item_remove(item);
    } else if (e.type == ReparentNotify) {
      //OnReparentNotify(e.xreparent);
    } else if (e.type == MapNotify) {
      if (e.xmap.window != overlay) {
        fprintf(stderr, "MapNotify %ld\n", e.xmap.window);
        Item *item = item_get_from_window(e.xmap.window);
        item->is_mapped = True;
        item->type->update(item);
        draw();
      }
   } else if (e.type == UnmapNotify) {
      Item *item = item_get_from_window(e.xunmap.window);
      item->is_mapped = False;
      draw();
    } else if (e.type == MapRequest) {
      XMapWindow(display, e.xmaprequest.window);
    } else if (e.type == ConfigureRequest) {
      //OnConfigureRequest(e.xconfigurerequest);
    } else if (e.type == PropertyNotify) {
      Bool handled = False;
      if (   (e.xproperty.window == root)
          && (e.xproperty.atom == IG_VIEWS)) {
        view_free_all(views);
        views = view_load_all();
        draw();
        handled=True;
      } else if (e.xproperty.atom == IG_COORDS) {
        Item *item = item_get_from_window(e.xproperty.window);
        item_type_window_update_space_pos_from_window(item);
        item->type->update(item);
        draw();
        handled=True;
      } else {
        for (View **v = views; *v; v++) {
          if (e.xproperty.atom == (*v)->attr_layer) {
            view_load_layer(*v);
            draw();
            handled=True;
          } else if (e.xproperty.atom == (*v)->attr_view) {
            view_load_screen(*v);
            draw();
            handled=True;
          }
        }
      }
      if (!handled) {
        print_xevent(display, &e);
        fprintf(stderr, "Ignored event\n"); fflush(stderr);
      }
    } else if (e.type == MotionNotify) {
      while (XCheckTypedWindowEvent(display, e.xmotion.window, MotionNotify, &e)) {}

      int winx, winy;
      Item *item;
      Window win = root;
      pick(e.xmotion.x_root, e.xmotion.y_root, &winx, &winy, &item);
      if (item && item_isinstance(item, &item_type_window)) {
        ItemWindow *window_item = (ItemWindow *) item;
        win = window_item->window;

        XWindowChanges values;
        values.x = e.xmotion.x_root - winx;
        values.y = e.xmotion.y_root - winy;
        values.stack_mode = Above;
        XConfigureWindow(display, window_item->window, CWX | CWY | CWStackMode, &values);

        if (debug_positions)
          printf("Point %d,%d -> %d,%d,%d\n", e.xmotion.x_root, e.xmotion.y_root, window_item->window, winx, winy); fflush(stdout);
      } else {
        if (debug_positions)
          printf("Point %d,%d -> NONE\n", e.xmotion.x_root, e.xmotion.y_root); fflush(stdout);
      }
/*
      XEvent ev = {0};
      ev.xclient.type = ClientMessage;
      ev.xclient.window = win;
      ev.xclient.message_type = IG_NOTIFY_MOTION;
      ev.xclient.format = 32;
      for (int i=0; i < 2 && views[i]; i++) {
        view_to_space(views[i],
                      e.xmotion.x_root, e.xmotion.y_root,
                      (float *) &ev.xclient.data.l[i*2], (float *) &ev.xclient.data.l[i*2+1]);
      }
      XSendEvent(display, win, False, PointerMotionMask, &ev);
*/

      size_t nrviews = 0;
      for (View **v = views; *v; v++, nrviews++);
      float coords[nrviews * 2];
      for (int i=0; i<nrviews; i++) {
        view_to_space(views[i],
                      e.xmotion.x_root, e.xmotion.y_root,
                      &coords[i*2], &coords[i*2+1]);
      }
      XChangeProperty(display, motion_notification_window, IG_NOTIFY_MOTION, XA_FLOAT, 32, PropModeReplace, &coords, 2*nrviews);
      XChangeProperty(display, motion_notification_window, IG_ACTIVE_WINDOW, XA_WINDOW, 32, PropModeReplace, &win, 1);
    } else if (e.type == ClientMessage && e.xclient.message_type == IG_EXIT) {
      exit(1);
    } else {
      print_xevent(display, &e);
      fprintf(stderr, "Ignored event\n"); fflush(stderr);
    }
  }
  return 0;
}
