#include<GL/gl.h>

typedef struct {
  GLuint program;
  GLchar *vertex_src, *fragment_src;
  GLuint vertex_shader, fragment_shader;
} Shader;

Shader *loadShader(char *vertex_src, char *fragment_src);
