#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XTest.h>

#include<GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "xapi.h"
#include "glapi.h"
#include "shader.h"
#include "space.h"
#include "input.h"

#include <SOIL/SOIL.h>

Shader *shader_program;
GLint sampler_attr;
GLint screen_attr;
unsigned int coords_attr;
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

void draw() {
  glClear(GL_COLOR_BUFFER_BIT);
  for (Item **itemp = items_all; itemp && *itemp; itemp++) {
    Item *item = *itemp;

    if (item->is_mapped) {
      item_update_texture(item);

      GLuint space_pos_vbo;
      GLfloat coords[4] = {0.3, 0.4, 0.5, 0.6};
      
      //fprintf(stderr, "%f,%f[%f,%f]\n", item->coords[0], item->coords[1], item->coords[2], item->coords[3]);
      glEnableVertexAttribArray(coords_attr);
      glBindBuffer(GL_ARRAY_BUFFER, item->coords_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);
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

int main() {
  if (!xinit()) return 1;
  if (!glinit(overlay)) return 1;
  if (!(shader_program = loadShader("vertex_shader.glsl", "geometry_shader.glsl", "fragment_shader.glsl"))) return 1;

  push_input_mode(&base_input_mode.base);

  initItems();

  glUseProgram(shader_program->program);
 
  glClearColor(1.0, 1.0, 0.5, 1.0);
  glViewport(overlay_attr.x, overlay_attr.y, overlay_attr.width, overlay_attr.height);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  
  screen[0] = 0.;
  screen[1] = 0.;
  screen[2] = 1.;
  screen[3] = (float) overlay_attr.height / (float) overlay_attr.width;
  
  screen_attr = glGetUniformLocation(shader_program->program, "screen");
  glUniform4fv(screen_attr, 1, screen);
  
  sampler_attr = glGetUniformLocation(shader_program->program, "myTextureSampler");
  coords_attr = glGetAttribLocation(shader_program->program, "coords");

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
