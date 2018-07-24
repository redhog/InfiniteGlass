#include "xapi.h"

t_glx_bind glXBindTexImageEXT = 0;
t_glx_release glXReleaseTexImageEXT = 0;

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

int xinit() {
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
  return 0;
 }

 XSetErrorHandler(&OnXError);

 int event_base, error_base;
 if (!XCompositeQueryExtension(display, &event_base, &error_base)) {
  fprintf(stderr, "X server does not support the Composite extension"); fflush(stderr);
  return 0;
 }
    
 int major = 0, minor = 3;
 XCompositeQueryVersion(display, &major, &minor);	
 if (major == 0 && minor < 3) {
  fprintf(stderr, "X server Composite extension is too old %i.%i < 0.3)", major, minor); fflush(stderr);
  return 0;
 }

 extensions = glXQueryExtensionsString(display, 0); fflush(stderr);

 printf("Extensions: %s\n", extensions);
 if(! strstr(extensions, "GLX_EXT_texture_from_pixmap")) {
  fprintf(stderr, "GLX_EXT_texture_from_pixmap not supported!\n");
  return 0;
 }

 glXBindTexImageEXT = (t_glx_bind) glXGetProcAddress((const GLubyte *)"glXBindTexImageEXT");
 glXReleaseTexImageEXT = (t_glx_release) glXGetProcAddress((const GLubyte *)"glXReleaseTexImageEXT");

 if(!glXBindTexImageEXT || !glXReleaseTexImageEXT) {
  fprintf(stderr, "Some extension functions missing!"); fflush(stderr);
  return 0;
 }
 
 overlay = XCompositeGetOverlayWindow(display, root);
 XGetWindowAttributes(display, overlay, &overlay_attr);

 XDamageQueryExtension(display, &damage_event, &damage_error);
 XShapeQueryExtension(display, &shape_event, &shape_error);
 
 return 1;
}
