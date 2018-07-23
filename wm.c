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
unsigned int space_pos_attr;
unsigned int win_pos_attr;

float win_pos[4][2] = {
  {0.0, 1.0},
  {0.0, 0.0},
  {1.0, 1.0},
  {1.0, 0.0}
};
GLuint win_pos_vbo;

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
    item_update_texture(item);
  }

  XFree(top_level_windows);
  XUngrabServer(display);
}

void draw() {
  glClear(GL_COLOR_BUFFER_BIT);
  for (Item **itemp = items_all; *itemp; itemp++) {
    Item *item = *itemp;
    
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
    fprintf(stderr, "Received event: %i", e.type);

    switch (e.type) {
      case CreateNotify:
       //OnCreateNotify(e.xcreatewindow);
       break;
      case DestroyNotify:
       //OnDestroyNotify(e.xdestroywindow);
       break;
      case ReparentNotify:
       //OnReparentNotify(e.xreparent);
       break;
      case MapNotify:
       //OnMapNotify(e.xmap);
       break;
      case UnmapNotify:
       //OnUnmapNotify(e.xunmap);
       break;
      case ConfigureNotify:
       //OnConfigureNotify(e.xconfigure);
       break;
      case MapRequest:
       //OnMapRequest(e.xmaprequest);
       break;
      case ConfigureRequest:
       //OnConfigureRequest(e.xconfigurerequest);
       break;
      case ButtonPress:
       //OnButtonPress(e.xbutton);
       break;
      case ButtonRelease:
       //OnButtonRelease(e.xbutton);
       break;
      case MotionNotify:
       while (XCheckTypedWindowEvent(display, e.xmotion.window, MotionNotify, &e)) {}
       // OnMotionNotify(e.xmotion);
       break;
      case KeyPress:
       //OnKeyPress(e.xkey);
       break;
      case KeyRelease:
       //OnKeyRelease(e.xkey);
       break;
      default:
       fprintf(stderr, "Ignored event\n"); fflush(stderr);
    }
  }
  return 0;
}
