#include "glapi.h"
#include "xapi.h"
#include "debug.h"

GLXFBConfig *configs;

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

int gl_check_error(char *msg) {
  GLenum errCode;
  const GLubyte *errString;

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    errString = gluErrorString(errCode);
    if (msg) {
      fprintf(stderr, "%s: OpenGL error %s\n", msg, errString);
    } else {
      fprintf(stderr, "OpenGL error %s\n", errString);
    }
    return 0;
  }
  return 1;
}

int glinit(Window window) { 
  int elements;
  int attrib_list[] = {GLX_X_RENDERABLE, True, 0};
  configs = glXChooseFBConfig(display, 0, attrib_list, &elements);

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

  if (GL_CHECK_ERROR("init", "Unable to initialize OpenGL context")) return 0;
  
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "GLEW: Unable to initialize: %s\n", glewGetErrorString(err));
    return 0;
  }
  DEBUG("init.glew", "GLEW: %s\n", glewGetString(GLEW_VERSION));
  
  const GLubyte* vendor   = glGetString(GL_VENDOR);
  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* version  = glGetString(GL_VERSION);
  const GLubyte* glsl_ver = glGetString(GL_SHADING_LANGUAGE_VERSION);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  if (GL_CHECK_ERROR("init", "Unable to set up OpenGL vertex arrays")) return 0;

  glViewport(0, 0, overlay_attr.width, overlay_attr.height);
  if (GL_CHECK_ERROR("init", "Unable to set up OpenGL viewport: %ld, %ld\n", overlay_attr.width, overlay_attr.height)) return 0;
/* FIXME: This code throws an error. Why?
  glFrustum(-1.,
 	1.,
 	-1.,
 	1.,
 	1.,
 	2.);
  if (GL_CHECK_ERROR("init", "Unable to set up OpenGL projection\n")) return 0;
*/
  
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_GEQUAL);
  if (GL_CHECK_ERROR("init", "Unable to set up OpenGL depth handling\n")) return 0;
  
  DEBUG("init.opengl", "OpenGL: %s:%s(%s)\nGLSL: %s\n", vendor, renderer, version, glsl_ver);
  return 1;
}
