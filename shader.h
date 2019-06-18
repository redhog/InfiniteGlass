#ifndef SHADER
#define SHADER

#include<GL/gl.h>

typedef struct {
  GLuint program;
  GLchar *vertex_src, *geometry_src, *fragment_src;
  GLuint vertex_shader, geometry_shader, fragment_shader;
} Shader;

Shader *shader_load(char *vertex_src, char *geometry_src, char *fragment_src);

#endif
