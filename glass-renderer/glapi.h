#ifndef GLAPI
#define GLAPI

#define GL_GLEXT_PROTOTYPES

#include <X11/Xutil.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "debug.h"

GLXFBConfig *configs;


#define GL_CHECK_ERROR(entry, ...) ({ \
  GLenum errCode; \
  const GLubyte *errString; \
  \
  if ((errCode = glGetError()) != GL_NO_ERROR) { \
    errString = gluErrorString(errCode); \
    if (ERROR_ENABLED(entry)) { \
      debug_print(stderr, 1, "GLASS_ERROR.renderer", __FILE__, __func__, entry, __VA_ARGS__); \
      fprintf(stderr, " OpenGL error: %s\n", errString); \
      BACKTRACE(entry); \
    } \
  }; \
  errCode != GL_NO_ERROR; \
})

int glinit(Window window);

#endif
