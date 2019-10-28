#ifndef SHADER
#define SHADER

#include "glapi.h"

typedef struct {
  Atom name;
  Atom geometry;
  Atom vertex;
  Atom fragment;

  GLchar *geometry_src;
  GLchar *vertex_src;
  GLchar *fragment_src;
 
  GLuint program;
  GLuint geometry_shader;
  GLuint vertex_shader;
  GLuint fragment_shader;
} Shader;

extern int shader_load(Shader *shader);

#endif
