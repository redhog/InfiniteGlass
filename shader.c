#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include "shader.h"
#include "glapi.h"

char *filetobuf(char *filename) {
  char *buffer = 0;
  long length;
  FILE *f = fopen(filename, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  length = ftell(f);
  fseek(f, 0, SEEK_SET);
  buffer = malloc(length + 1);
  if (buffer) {
    fread(buffer, 1, length, f);
    buffer[length] = 0;
  }
  fclose(f);

  return buffer;
}

int checkShaderError(char *name, char *src, GLuint shader) {
  GLint res;
  GLint len;
  GLchar *log;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
  if (res == GL_TRUE) return 1;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
  log = malloc(len + 1);
  glGetShaderInfoLog(shader, len, &len, log);
  fprintf(stderr, "%s shader compilation failed: %s [%d]\n\n%s\n\n", name, log, len, src);
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
  fprintf(stderr, "Program linkage failed: %s [%d]\n", log, len);
  gl_check_error("checkProgramError");
  return 0;
}

Shader *shader_load(char *vertex_src, char *geometry_src, char *fragment_src) {
  Shader *res = (Shader *) malloc(sizeof(Shader));
 
  res->vertex_src = filetobuf(vertex_src);
  res->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(res->vertex_shader, 1, (const GLchar**)&(res->vertex_src), 0);
  glCompileShader(res->vertex_shader);
  if (!checkShaderError("vertex", res->vertex_src, res->vertex_shader)) { free(res); return NULL; }

  res->geometry_src = filetobuf(geometry_src);
  res->geometry_shader = glCreateShader(GL_GEOMETRY_SHADER_ARB);
  if (!gl_check_error("shader_load")) { free(res); return NULL; }
  glShaderSource(res->geometry_shader, 1, (const GLchar**)&(res->geometry_src), 0);
  glCompileShader(res->geometry_shader);
  if (!checkShaderError("geometry", res->geometry_src, res->geometry_shader)) { free(res); return NULL; }

  res->fragment_src = filetobuf(fragment_src);
  res->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(res->fragment_shader, 1, (const GLchar**)&(res->fragment_src), 0);
  glCompileShader(res->fragment_shader);
  if (!checkShaderError("fragment", res->fragment_src, res->fragment_shader)) { free(res); return NULL; }

  res->program = glCreateProgram();

  glAttachShader(res->program, res->vertex_shader);
  glAttachShader(res->program, res->geometry_shader);
  glAttachShader(res->program, res->fragment_shader);

  // glBindAttribLocation(res->program, 1, "in_Position");

  glLinkProgram(res->program);
  if (!checkProgramError(res->program)) { free(res); return NULL; }
  
  return res;
}
