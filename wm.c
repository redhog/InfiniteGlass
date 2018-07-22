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


int main() {
 int res;
 
 if ((res = xinit()) != 0) return res;
 
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
 
 if ((res = glinit(overlay)) != 0) return res;
 
 glShadeModel(GL_FLAT);
 glClearColor(0.5, 0.5, 0.5, 1.0);

 glViewport(overlay_attr.x, overlay_attr.y, overlay_attr.width, overlay_attr.height);
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

 glClear(GL_COLOR_BUFFER_BIT);
 glColor3f(1.0, 1.0, 0.0);
 glRectf(-1., -1., 1., 1.);

 for (unsigned int i = 0; i < num_top_level_windows; ++i) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, top_level_windows[i], &attr);
  
  int x                     = attr.x;
  int y                     = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  float left = ((float) (x - overlay_attr.x)) / (float) overlay_attr.width;
  float right = left + (float) width / (float) overlay_attr.width;
  float top = ((float) y - overlay_attr.y) / (float) overlay_attr.height;
  float bottom = top + (float) height / (float) overlay_attr.height;

  left = 2. * left - 1.; 
  right = 2. * right - 1.; 
  top = 1 - 2. * top; 
  bottom = 1. - 2. * bottom; 
  
  fprintf(stderr, "%u: %i,%i(%i,%i)\n", (uint) top_level_windows[i], x, y, width, height); fflush(stderr);
 
  GLXPixmap glxpixmap = 0;
  GLuint texture_id;

  Pixmap pixmap = XCompositeNameWindowPixmap(display, top_level_windows[i]);
  const int pixmap_attribs[] = {
   GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
   GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
   None
  };
  glxpixmap = glXCreatePixmap(display, configs[0], pixmap, pixmap_attribs);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glXBindTexImageEXT(display, glxpixmap, GLX_FRONT_EXT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);



  
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0); glVertex3f(left,  top,    0.0);
  glTexCoord2f(1.0, 0.0); glVertex3f(right, top,    0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f(right, bottom, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(left,  bottom, 0.0);
  glEnd();

  XFreePixmap(display, pixmap);
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
