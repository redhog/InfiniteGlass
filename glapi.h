#ifndef GLAPI
#define GLAPI

#include <X11/Xutil.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

GLXFBConfig *configs;

int checkError(char *msg);
int glinit(Window window);

#endif
