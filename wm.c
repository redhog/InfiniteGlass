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

Shader *shaderProgram;

int main() {
  if (!xinit()) return 1;
  if (!glinit(overlay)) return 1;
  if (!(shaderProgram = loadShader("vertex_shader.glsl", "fragment_shader.glsl"))) return 1;
  glUseProgram(shaderProgram->program);
 
  glClearColor(1.0, 1.0, 0.5, 1.0);
  glViewport(overlay_attr.x, overlay_attr.y, overlay_attr.width, overlay_attr.height);
  glClear(GL_COLOR_BUFFER_BIT);

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

  GLint samplerLoc = glGetUniformLocation(shaderProgram->program, "myTextureSampler");
  unsigned int space_pos_attr = glGetAttribLocation(shaderProgram->program, "space_pos");
  unsigned int win_pos_attr = glGetAttribLocation(shaderProgram->program, "win_pos");

  float win_pos[4][2] = {
    {0.0, 0.0},
    {0.0, 1.0},
    {1.0, 0.0},
    {1.0, 1.0}
  };

  GLuint win_pos_vbo;
  glGenBuffers(1, &win_pos_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, win_pos_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(win_pos), win_pos, GL_STATIC_DRAW);
  glVertexAttribPointer(win_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(win_pos_attr);

  for (unsigned int i = 0; i < num_top_level_windows; ++i) {
    Item *item = item_get(top_level_windows[i]);
    item_update_space_pos(item);
    item_update_texture(item);
    
    GLuint space_pos_vbo;
    
    glGenBuffers(1, &space_pos_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, space_pos_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(item->space_pos), item->space_pos, GL_STATIC_DRAW);
    glVertexAttribPointer(space_pos_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(space_pos_attr);
    
    glUniform1i(samplerLoc, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, item->texture_id);
    glBindSampler(0, 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glFlush();
  }

  glXSwapBuffers(display, overlay);

  XFree(top_level_windows);
  XUngrabServer(display);

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
