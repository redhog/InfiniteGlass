#ifndef SHADER
#define SHADER

#include<GL/gl.h>

typedef struct {
  GLchar *vertex_src_file;
  GLchar *geometry_src_file;
  GLchar *fragment_src_file;

  GLchar *vertex_src;
  GLchar *geometry_src;
  GLchar *fragment_src;
 
  GLuint program;
  GLuint vertex_shader;
  GLuint geometry_shader;
  GLuint fragment_shader;
} Shader;

extern int shader_load(Shader *shader);

#endif
