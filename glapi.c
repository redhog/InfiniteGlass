#include "glapi.h"
#include "xapi.h"

int glinit(Window window) { 
  int elements;
  configs = glXChooseFBConfig(display, 0, NULL, &elements);
  GLXContext context = glXCreateNewContext(display, configs[0], GLX_RGBA_TYPE, NULL, True);
  glXMakeCurrent(display, window, context);

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err));
    return 0;
  }
  fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
  return 1;
}
