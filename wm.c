#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "xapi.h"
#include "glapi.h"
#include "shader.h"
#include "space.h"
#include "input.h"
#include "wm.h"

#include <SOIL/SOIL.h>

Shader *shader_program;
GLint sampler_attr;
GLint screen_attr;
GLint coords_attr;
GLint picking_mode_attr;
GLint window_id_attr;

float screen[4];

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
    Item *item = item_get(top_level_windows[i]);
    XGetWindowAttributes(display, top_level_windows[i], &attr);
    item->is_mapped = attr.map_state == IsViewable;
    item_update_space_pos_from_window(item);
    item_update_pixmap(item);
  }

  XFree(top_level_windows);
  XUngrabServer(display);
}

void abstract_draw() {
  glClear(GL_COLOR_BUFFER_BIT);
  glUniform4fv(screen_attr, 1, screen);
  for (Item **itemp = items_all; itemp && *itemp; itemp++) {
    Item *item = *itemp;

    if (item->is_mapped) {
      glUniform1f(window_id_attr, (float) item->window / (float) INT_MAX);
     
      item_update_texture(item);

      glEnableVertexAttribArray(coords_attr);
      glBindBuffer(GL_ARRAY_BUFFER, item->coords_vbo);
      glVertexAttribPointer(coords_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);

      glUniform1i(sampler_attr, 0);
      glActiveTexture(GL_TEXTURE0 + 0);
      glBindTexture(GL_TEXTURE_2D, item->texture_id);
      glBindSampler(0, 0);

      glDrawArrays(GL_POINTS, 0, 1);
      glFlush();
    }
  }

  glXSwapBuffers(display, overlay);
}

void draw() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUniform1i(picking_mode_attr, 1);
  abstract_draw();
}
GLint picking_fb;

void pick(int x, int y, float *winx, float *winy, Item **item) {
  float data[4];
  memset(data, 0, sizeof(data));
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  glUniform1i(picking_mode_attr, 1);
  abstract_draw();
  glReadPixels(x, overlay_attr.height - y, 1, 1,
               GL_RGBA, GL_FLOAT,
               (GLvoid *) data);
//  checkError();
  fprintf(stderr, "Pick %d,%d -> %f,%f,%f,%f\n", x, y, data[0], data[1], data[2], data[3]);
  return;
  if (data[2] == 0.0) {
    *winx = 0;
    *winy = 0;
//    *item = NULL;
  } else {
    *winx = data[0];
    *winy = data[1];
//    *item = item_get((Window) (data[2] * (float) INT_MAX));
  }
//  fprintf(stderr, "Pick %d,%d -> %d,%f,%f\n", x, y, (int) (data[2] * (float) INT_MAX), *winx, *winy);
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
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 256, 256);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
   fprintf(stderr, "Unable to create picking framebuffer.\n");
    return 0;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, picking_fb);
  return 1;
}

int main() {
  if (!xinit()) return 1;
  if (!glinit(overlay)) return 1;
  if (!(shader_program = loadShader("shader_window_vertex.glsl", "shader_window_geometry.glsl", "shader_window_fragment.glsl"))) return 1;
  if (!init_picking()) return 1;
  
  fprintf(stderr, "Initialized X and GL.\n");

  glUseProgram(shader_program->program);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  screen[0] = 0.;
  screen[1] = 0.;
  screen[2] = 1.;
  screen[3] = (float) overlay_attr.height / (float) overlay_attr.width;
  
  screen_attr = glGetUniformLocation(shader_program->program, "screen");
  sampler_attr = glGetUniformLocation(shader_program->program, "myTextureSampler");
  coords_attr = glGetAttribLocation(shader_program->program, "coords");
  picking_mode_attr = glGetUniformLocation(shader_program->program, "pickingMode");
  window_id_attr = glGetUniformLocation(shader_program->program, "windowId");
  
  push_input_mode(&base_input_mode.base);

  initItems();
 
  glClearColor(1.0, 1.0, 0.5, 1.0);
  glViewport(overlay_attr.x, overlay_attr.y, overlay_attr.width, overlay_attr.height);  
  
  draw();

  for (;;) {
    XEvent e;
    XNextEvent(display, &e);
    //fprintf(stderr, "Received event: %i\n", e.type);

    if (e.type == damage_event + XDamageNotify) {
     //fprintf(stderr, "Received XDamageNotify\n");
      XDamageNotifyEvent *event = (XDamageNotifyEvent*) &e;
      // e->drawable is the window ID of the damaged window
      // e->geometry is the geometry of the damaged window	
      // e->area     is the bounding rect for the damaged area	
      // e->damage   is the damage handle returned by XDamageCreate()

      // Subtract all the damage, repairing the window.
      draw();
      XDamageSubtract(display, event->damage, None, None);
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
    } else if (e.type == ConfigureNotify) {
     fprintf(stderr, "Received ConfigureNotify for %ld\n", e.xconfigure.window);
      Item *item = item_get(e.xconfigure.window);
      item_update_space_pos_from_window(item);
      item_update_pixmap(item);
      draw();
    } else if (e.type == CreateNotify) {
      if (e.xcreatewindow.window != overlay) {
        fprintf(stderr, "CreateNotify %ld under %ld @ %d,%d size = %d, %d\n", e.xcreatewindow.window, e.xcreatewindow.parent, e.xcreatewindow.x, e.xcreatewindow.y, e.xcreatewindow.width, e.xcreatewindow.height);
        Item *item = item_get(e.xcreatewindow.window);
        XMapWindow(display, e.xmaprequest.window);
        item_update_space_pos_from_window(item);
        item_update_pixmap(item);
      }
    } else if (e.type == DestroyNotify) {
     item_remove(item_get(e.xdestroywindow.window));
    } else if (e.type == ReparentNotify) {
      //OnReparentNotify(e.xreparent);
    } else if (e.type == MapNotify) {
      if (e.xmap.window != overlay) {
        fprintf(stderr, "MapNotify %ld\n", e.xmap.window);
        Item *item = item_get(e.xmap.window);
        item->is_mapped = True;
        item_update_pixmap(item);
        draw();
      }
   } else if (e.type == UnmapNotify) {
      Item *item = item_get(e.xunmap.window);
      item->is_mapped = False;
      draw();
    } else if (e.type == MapRequest) {
      XMapWindow(display, e.xmaprequest.window);
      fprintf(stderr, "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX: %ld\n", e.xmaprequest.window);
      //OnMapRequest(e.xmaprequest);
    } else if (e.type == ConfigureRequest) {
      //OnConfigureRequest(e.xconfigurerequest);
    } else if (e.type == ButtonPress) {
      input_mode_stack_handle(e);
    } else if (e.type == ButtonRelease) {
      fprintf(stderr, "ButtonRelease of %i @ %d,%d with mask %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
              e.xbutton.button,
              e.xbutton.x,
              e.xbutton.y,
              e.xbutton.state & Button1Mask ? "Button1, " : "",
              e.xbutton.state & Button2Mask ? "Button2, " : "",
              e.xbutton.state & Button3Mask ? "Button3, " : "",
              e.xbutton.state & Button4Mask ? "Button4, " : "",
              e.xbutton.state & Button5Mask ? "Button5, " : "",
              e.xbutton.state & ShiftMask ? "Shift, " : "",
              e.xbutton.state & LockMask ? "Lock, " : "",
              e.xbutton.state & ControlMask ? "Control, " : "",
              e.xbutton.state & Mod1Mask ? "Mod1, " : "",
              e.xbutton.state & Mod2Mask ? "Mod2, " : "",
              e.xbutton.state & Mod3Mask ? "Mod3, " : "",
              e.xbutton.state & Mod4Mask ? "Mod4, " : "",
              e.xbutton.state & Mod5Mask ? "Mod5, " : "");
      input_mode_stack_handle(e);
    } else if (e.type == MotionNotify) {
      while (XCheckTypedWindowEvent(display, e.xmotion.window, MotionNotify, &e)) {}
      input_mode_stack_handle(e);
    } else if (e.type == KeyPress) {
      input_mode_stack_handle(e);
    } else if (e.type == KeyRelease) {
     fprintf(stderr, "KeyRelease of %i @ %d,%d with mask %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
             e.xkey.keycode,
             e.xkey.x,
             e.xkey.y,
             e.xkey.state & Button1Mask ? "Button1, " : "",
             e.xkey.state & Button2Mask ? "Button2, " : "",
             e.xkey.state & Button3Mask ? "Button3, " : "",
             e.xkey.state & Button4Mask ? "Button4, " : "",
             e.xkey.state & Button5Mask ? "Button5, " : "",
             e.xkey.state & ShiftMask ? "Shift, " : "",
             e.xkey.state & LockMask ? "Lock, " : "",
             e.xkey.state & ControlMask ? "Control, " : "",
             e.xkey.state & Mod1Mask ? "Mod1, " : "",
             e.xkey.state & Mod2Mask ? "Mod2, " : "",
             e.xkey.state & Mod3Mask ? "Mod3, " : "",
             e.xkey.state & Mod4Mask ? "Mod4, " : "",
             e.xkey.state & Mod5Mask ? "Mod5, " : "");
      input_mode_stack_handle(e);
    } else {
      fprintf(stderr, "Ignored event\n"); fflush(stderr);
    }
  }
  return 0;
}
