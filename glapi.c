#include "glapi.h"
#include "xapi.h"

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

int glinit(Window window) { 
  int elements;
  configs = glXChooseFBConfig(display, 0, NULL, &elements);

  int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                           GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                           GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                           None};

  glXCreateContextAttribsARBProc glXCreateContextAttribsARB =
    (glXCreateContextAttribsARBProc) glXGetProcAddressARB((const GLubyte *) "glXCreateContextAttribsARB");
  
  GLXContext context = glXCreateContextAttribsARB(display, configs[0], 0, True, context_attribs);
  XSync(display, False);
  if (!context) {
    fprintf(stderr, "GLX: Unable to initialize.\n");
    return 0;
  }
  glXMakeCurrent(display, window, context);

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "GLEW: Unable to initialize: %s\n", glewGetErrorString(err));
    return 0;
  }
  fprintf(stdout, "GLEW: %s\n", glewGetString(GLEW_VERSION));
  
  const GLubyte* vendor   = glGetString(GL_VENDOR);
  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* version  = glGetString(GL_VERSION);
  const GLubyte* glsl_ver = glGetString(GL_SHADING_LANGUAGE_VERSION);
  printf("OpenGL: %s:%s(%s)\nGLSL: %s\n", vendor, renderer, version, glsl_ver);
  return 1;
}
