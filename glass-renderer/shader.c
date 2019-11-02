#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shader.h"
#include "glapi.h"
#include "xapi.h"
#include "debug.h"

int checkShaderError(char *name, char *src, GLuint shader) {
  GLint res;
  GLint len;
  GLchar *log;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
  if (res == GL_TRUE) return 1;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
  log = malloc(len + 1);
  glGetShaderInfoLog(shader, len, &len, log);
  DEBUG("shader", "%s shader compilation failed: %s [%d]\n\n%s\n\n", name, log, len, src);
  gl_check_error(name);
  return 0;
}

int checkProgramError(GLuint program) {
  GLint res;
  GLint len;
  GLchar *log;
  glGetProgramiv(program, GL_LINK_STATUS, &res);
  if (res == GL_TRUE) return 1;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
  log = malloc(len + 1);
  glGetProgramInfoLog(program, len, &len, log);
  DEBUG("shader", "Program linkage failed: %s [%d]\n", log, len);
  gl_check_error("checkProgramError");
  return 0;
}

char *atom_load_string(Display *display, Window window, Atom name) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;
  
  XGetWindowProperty(display, window, name, 0, 0, 0, XA_STRING, &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  XFree(prop_return);
  if (type_return == None) return NULL;
  XGetWindowProperty(display, window, name, 0, bytes_after_return, 0, XA_STRING,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  return (char *) prop_return;
}

Shader *shader_loadX(Atom name) {
  Shader *shader = malloc(sizeof(Shader));
  shader->name = name;
 
  shader->geometry = atom_append(display, shader->name, "_GEOMETRY");
  shader->vertex = atom_append(display, shader->name, "_VERTEX");
  shader->fragment = atom_append(display, shader->name, "_FRAGMENT");

  shader->geometry_src = atom_load_string(display, root, shader->geometry);
  shader->geometry_shader = glCreateShader(GL_GEOMETRY_SHADER_ARB);
  if (!gl_check_error("shader_load")) {
    XFree(shader->geometry_src);
    free(shader);
    return NULL;
  }
  glShaderSource(shader->geometry_shader, 1, (const GLchar**)&(shader->geometry_src), 0);
  glCompileShader(shader->geometry_shader);
  if (!checkShaderError("geometry", shader->geometry_src, shader->geometry_shader)) {
    XFree(shader->geometry_src);
    free(shader);
    return NULL;
  }

  shader->vertex_src = atom_load_string(display, root, shader->vertex);
  shader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shader->vertex_shader, 1, (const GLchar**)&(shader->vertex_src), 0);
  glCompileShader(shader->vertex_shader);
  if (!checkShaderError("vertex", shader->vertex_src, shader->vertex_shader)) {
    XFree(shader->geometry_src);
    XFree(shader->vertex_src);
    free(shader);
    return NULL;
  }

  shader->fragment_src = atom_load_string(display, root, shader->fragment);
  shader->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shader->fragment_shader, 1, (const GLchar**)&(shader->fragment_src), 0);
  glCompileShader(shader->fragment_shader);
  if (!checkShaderError("fragment", shader->fragment_src, shader->fragment_shader)) {
    XFree(shader->geometry_src);
    XFree(shader->vertex_src);
    XFree(shader->fragment_src);
    free(shader);
    return NULL;
  }

  shader->program = glCreateProgram();

  glAttachShader(shader->program, shader->geometry_shader);
  glAttachShader(shader->program, shader->vertex_shader);
  glAttachShader(shader->program, shader->fragment_shader);

  glLinkProgram(shader->program);
  if (!checkProgramError(shader->program)) {
    XFree(shader->geometry_src);
    XFree(shader->vertex_src);
    XFree(shader->fragment_src);
    free(shader);
    return NULL;
  }

  return shader;
}


List *shader_load_all(void) {
  Atom type_return;
  int format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *prop_return;

  XGetWindowProperty(display, root, XInternAtom(display, "IG_SHADERS", False), 0, 100000, 0, AnyPropertyType,
                     &type_return, &format_return, &nitems_return, &bytes_after_return, &prop_return);
  if (type_return == None) {
    XFree(prop_return);
    return NULL;
  }
  
  List *res = list_create();
  
  for (int i=0; i < nitems_return; i++) {
    list_append(res, (void *) shader_loadX(((Atom *) prop_return)[i]));
  }
  XFree(prop_return);

  if (DEBUG_ENABLED("shaders")) {
    DEBUG("shaders", "Shaders:\n");
    for (size_t idx = 0; idx < res->count; idx++) {
      shader_print((Shader *) res->entries[idx]);
    }
  }
  
  return res;
}

void shader_free(Shader *shader) {
  XFree(shader->geometry_src);
  XFree(shader->vertex_src);
  XFree(shader->fragment_src);
  free(shader);
}

void shader_free_all(List *shaders) {
  for (size_t idx = 0; idx < shaders->count; idx++) {
   shader_free((Shader *) shaders->entries[idx]);
  }
  list_destroy(shaders);
}

Shader *shader_find(List *shaders, Atom name) {
  for (size_t idx = 0; idx < shaders->count; idx++) {
    Shader *s = (Shader *) shaders->entries[idx];   
    if (s->name == name) {
      return s;
    }
  }
  return NULL;
}

void shader_print(Shader *shader) {
  fprintf(stderr,
          "%s:\nGeometry\n%s\n\nVertex:\n%s\n\nFragment:\n%s\n\n",
          XGetAtomName(display, shader->name),
          shader->geometry_src,
          shader->vertex_src,
          shader->fragment_src);
}

