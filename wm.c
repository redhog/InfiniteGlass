#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Atom WM_PROTOCOLS;
Atom WM_DELETE_WINDOW;

Display* display;
Window root;
Window overlay;
Bool wm_detected;
const char *extensions;
typedef void (*t_glx_bind)(Display *, GLXDrawable, int , const int *);
typedef void (*t_glx_release)(Display *, GLXDrawable, int);
t_glx_bind glXBindTexImageEXT = 0;
t_glx_release glXReleaseTexImageEXT = 0;

const int pixmap_attribs[] = {
 GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
 GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
 None
};

int OnWMDetected(Display* display, XErrorEvent* e) {
 wm_detected = True;
 return 0;
}

int OnXError(Display* display, XErrorEvent* e) {
 const int MAX_ERROR_TEXT_LENGTH = 1024;
 char error_text[MAX_ERROR_TEXT_LENGTH];
 XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
 fprintf(stderr,
         "Received X error:\n"
         "    Request: %i\n"
         "    Error code: %i - %s\n"
         "    Resource ID: %u\n",
         e->request_code,
         e->error_code,
         error_text,
         (uint) e->resourceid);
 return 0;
}


int main() {
 display = XOpenDisplay(NULL);
 root = DefaultRootWindow(display);
 WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
 WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);

 wm_detected = False;
 XSetErrorHandler(&OnWMDetected);
 XSelectInput(display, root, SubstructureRedirectMask | SubstructureNotifyMask);
 XSync(display, False);
 if (wm_detected) {
  fprintf(stderr, "Another window manager is already running"); fflush(stderr);
  return 1;
 }

 XSetErrorHandler(&OnXError);

 int event_base, error_base;
 if (!XCompositeQueryExtension(display, &event_base, &error_base)) {
  fprintf(stderr, "X server does not support the Composite extension"); fflush(stderr);
  return 1;
 }
    
 int major = 0, minor = 3;
 XCompositeQueryVersion(display, &major, &minor);	
 if (major == 0 && minor < 3) {
  fprintf(stderr, "X server Composite extension is too old %i.%i < 0.3)", major, minor); fflush(stderr);
  return 1;
 }

 extensions = glXQueryExtensionsString(display, 0); fflush(stderr);

 printf("Extensions: %s\n", extensions);
 if(! strstr(extensions, "GLX_EXT_texture_from_pixmap")) {
  fprintf(stderr, "GLX_EXT_texture_from_pixmap not supported!\n");
  return 1;
 }

 glXBindTexImageEXT = (t_glx_bind) glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
 glXReleaseTexImageEXT = (t_glx_release) glXGetProcAddress((const GLubyte *)"glXReleaseTexImageEXT");

 if(!glXBindTexImageEXT || !glXReleaseTexImageEXT) {
  fprintf(stderr, "Some extension functions missing!"); fflush(stderr);
  return 1;
 }

 
 overlay = XCompositeGetOverlayWindow(display, root);

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


 int elements;
 GLXFBConfig *configs = glXChooseFBConfig(display, 0, NULL, &elements);
 GLXContext context = glXCreateNewContext(display, configs[0], GLX_RGBA_TYPE, NULL, True);
 glXMakeCurrent(display, overlay, context);

 glShadeModel(GL_FLAT);
 glClearColor(0.5, 0.5, 0.5, 1.0);

 glViewport(0, 0, 200, 200);
 glMatrixMode(GL_PROJECTION);
 glLoadIdentity();
 glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

 glClear(GL_COLOR_BUFFER_BIT);
 glColor3f(1.0, 1.0, 0.0);
 glRectf(-0.8, -0.8, 0.8, 0.8);

 for (unsigned int i = 0; i < num_top_level_windows; ++i) {
  XWindowAttributes attr;
  XGetWindowAttributes(display, top_level_windows[i], &attr);
  
  XRenderPictFormat *format = XRenderFindVisualFormat(display, attr.visual);
  Bool hasAlpha             = (format->type == PictTypeDirect && format->direct.alphaMask);
  int x                     = attr.x;
  int y                     = attr.y;
  int width                 = attr.width;
  int height                = attr.height;
  
  fprintf(stderr, "%u: %i,%i(%i,%i)\n", (uint) top_level_windows[i], x, y, width, height); fflush(stderr);
 
  GLXPixmap glxpixmap = 0;
  GLuint texture_id;

  Pixmap pixmap = XCompositeNameWindowPixmap(display, top_level_windows[i]);
  glxpixmap = glXCreatePixmap(display, configs[0], pixmap, pixmap_attribs);

  glEnable(GL_TEXTURE_2D);
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glXBindTexImageEXT(display, glxpixmap, GLX_FRONT_EXT, NULL);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0); glVertex3f(-1.0,  1.0, 0.0);
  glTexCoord2f(1.0, 0.0); glVertex3f( 1.0,  1.0, 0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f( 1.0, -1.0, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, -1.0, 0.0);
  glEnd();  
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
