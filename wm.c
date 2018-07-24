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

#include <SOIL/SOIL.h>

Shader *shader_program;
GLint sampler_attr;
GLint screen_attr;
GLint zoom_pan_attr;
unsigned int space_pos_attr;
unsigned int win_pos_attr;

float win_pos[4][2] = {
  {0.0, 1.0},
  {0.0, 0.0},
  {1.0, 1.0},
  {1.0, 0.0}
};
GLuint win_pos_vbo;

GLfloat zoom_pan[16] = {
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0,
};
GLfloat screen[16] = {
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0,
};


void initItems() {
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
    item_update_space_pos_from_window(item);
    item_update_pixmap(item);
  }

  XFree(top_level_windows);
  XUngrabServer(display);
}

void draw() {
  glClear(GL_COLOR_BUFFER_BIT);
  for (Item **itemp = items_all; *itemp; itemp++) {
    Item *item = *itemp;

    item_update_texture(item);
    
    GLuint space_pos_vbo;
    
    glBindBuffer(GL_ARRAY_BUFFER, item->space_pos_vbo);
    glVertexAttribPointer(space_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(space_pos_attr);
    
    glUniform1i(sampler_attr, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, item->texture_id);
    glBindSampler(0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFlush();
  }

  glXSwapBuffers(display, overlay);
}

int main() {
  if (!xinit()) return 1;
  if (!glinit(overlay)) return 1;
  if (!(shader_program = loadShader("vertex_shader.glsl", "fragment_shader.glsl"))) return 1;

  initItems();

  glUseProgram(shader_program->program);
 
  glClearColor(1.0, 1.0, 0.5, 1.0);
  glViewport(overlay_attr.x, overlay_attr.y, overlay_attr.width, overlay_attr.height);

  zoom_pan_attr = glGetUniformLocation(shader_program->program, "zoom_pan");
  screen_attr = glGetUniformLocation(shader_program->program, "screen");

  glUniformMatrix4fvARB(zoom_pan_attr, 1, True, zoom_pan);
  glUniformMatrix4fvARB(screen_attr, 1, True, screen);
  
  sampler_attr = glGetUniformLocation(shader_program->program, "myTextureSampler");
  space_pos_attr = glGetAttribLocation(shader_program->program, "space_pos");
  win_pos_attr = glGetAttribLocation(shader_program->program, "win_pos");

  glGenBuffers(1, &win_pos_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, win_pos_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(win_pos), win_pos, GL_STATIC_DRAW);
  glVertexAttribPointer(win_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(win_pos_attr);

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
    } else if (e.type == ConfigureNotify) {
     //fprintf(stderr, "Received ConfigureNotify\n");
      XConfigureEvent *event = &e.xconfigure;
    } else if (e.type == CreateNotify) {
      //OnCreateNotify(e.xcreatewindow);
    } else if (e.type == DestroyNotify) {
      //OnDestroyNotify(e.xdestroywindow);
    } else if (e.type == ReparentNotify) {
      //OnReparentNotify(e.xreparent);
    } else if (e.type == MapNotify) {
      //OnMapNotify(e.xmap);
    } else if (e.type == UnmapNotify) {
      //OnUnmapNotify(e.xunmap);
    } else if (e.type == MapRequest) {
      //OnMapRequest(e.xmaprequest);
    } else if (e.type == ConfigureRequest) {
      //OnConfigureRequest(e.xconfigurerequest);
    } else if (e.type == ButtonPress) {
      //OnButtonPress(e.xbutton);
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
      //OnButtonRelease(e.xbutton);
    } else if (e.type == MotionNotify) {
      while (XCheckTypedWindowEvent(display, e.xmotion.window, MotionNotify, &e)) {}
      // OnMotionNotify(e.xmotion);
    } else if (e.type == KeyPress) {
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
    } else {
      fprintf(stderr, "Ignored event\n"); fflush(stderr);
    }
  }
  return 0;
}
