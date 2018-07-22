#include<GL/glew.h>
#include<stdlib.h>
#include<stdio.h>
#include "shader.h"

char *filetobuf(char *filename) {
  char *buffer = 0;
  long length;
  FILE *f = fopen(filename, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  length = ftell(f);
  fseek(f, 0, SEEK_SET);
  buffer = malloc(length);
  if (buffer) {
    fread(buffer, 1, length, f);
  }
  fclose(f);

  return buffer;
}

int checkError() {
  GLenum errCode;
  const GLubyte *errString;

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    errString = gluErrorString(errCode);
    fprintf(stderr, "OpenGL error %s\n", errString);
    return 0;
  }
  return 1;
}

int checkShaderError(GLuint shader) {
  GLint res;
  GLint len;
  GLchar *log;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
  if (res == GL_TRUE) return 1;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
  log = malloc(len);
  glGetShaderInfoLog(shader, len, &len, log);
  fprintf(stderr, "Shader compilation failed: %s\n", log);
  return 0;
}

Shader *loadShader(char *vertex_src, char *fragment_src) {
  Shader *res = (Shader *) malloc(sizeof(Shader));
 
  res->vertex_src = filetobuf(vertex_src);
  res->fragment_src = filetobuf(fragment_src);

  res->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  res->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(res->vertex_shader, 1, (const GLchar**)&(res->vertex_src), 0);
  glShaderSource(res->fragment_shader, 1, (const GLchar**)&(res->fragment_src), 0);

  glCompileShader(res->vertex_shader);
  if (!checkShaderError(res->vertex_shader)) { free(res); return NULL; }
  glCompileShader(res->fragment_shader);
  if (!checkShaderError(res->fragment_shader)) { free(res); return NULL; }

  res->program = glCreateProgram();

  glAttachShader(res->program, res->vertex_shader);
  glAttachShader(res->program, res->fragment_shader);

  // glBindAttribLocation(res->program, 1, "in_Position");

  glLinkProgram(res->program);
  if (!checkError()) {
    free(res);
    return NULL;
  }

  
  return res;
}
