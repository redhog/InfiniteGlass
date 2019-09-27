#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shader.h"
#include "glapi.h"
#include "debug.h"

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

char *pathfiletobuf(char *filename) {
  char *path;
  char *next; 
  char *res; 

  for (path = getenv("GLASS_SHADER_PATH"); path[0]; path = next) {
    next = strstr(path, ":");
    if (!next) next = path + strlen(path);

    char pathentry[next - path + 1 + strlen(filename) + 1];
    strncpy(pathentry, path, next - path);
    pathentry[next - path] =  '/';
    strcpy(pathentry + (next - path) + 1, filename);

    res = filetobuf(pathentry);
    if (res) return res;
  }
  return NULL;
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

int shader_load(Shader *shader) {
  shader->vertex_src = pathfiletobuf(shader->vertex_src_file);
  shader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(shader->vertex_shader, 1, (const GLchar**)&(shader->vertex_src), 0);
  glCompileShader(shader->vertex_shader);
  if (!checkShaderError("vertex", shader->vertex_src, shader->vertex_shader)) return 0;

  shader->geometry_src = pathfiletobuf(shader->geometry_src_file);
  shader->geometry_shader = glCreateShader(GL_GEOMETRY_SHADER_ARB);
  if (!gl_check_error("shader_load")) return 0;
  glShaderSource(shader->geometry_shader, 1, (const GLchar**)&(shader->geometry_src), 0);
  glCompileShader(shader->geometry_shader);
  if (!checkShaderError("geometry", shader->geometry_src, shader->geometry_shader)) return 0;

  shader->fragment_src = pathfiletobuf(shader->fragment_src_file);
  shader->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shader->fragment_shader, 1, (const GLchar**)&(shader->fragment_src), 0);
  glCompileShader(shader->fragment_shader);
  if (!checkShaderError("fragment", shader->fragment_src, shader->fragment_shader)) return 0;

  shader->program = glCreateProgram();

  glAttachShader(shader->program, shader->vertex_shader);
  glAttachShader(shader->program, shader->geometry_shader);
  glAttachShader(shader->program, shader->fragment_shader);

  // glBindAttribLocation(shader->program, 1, "in_Position");

  glLinkProgram(shader->program);
  if (!checkProgramError(shader->program)) return 0;

  return 1;
}
