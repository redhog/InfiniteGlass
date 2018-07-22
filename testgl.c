#include<GL/glew.h>
#include<GL/glut.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glu.h>
#include<stdlib.h>
#include <stdio.h>
#include "shader.h"
#include <unistd.h>

GLuint triangleVBO;
Shader *shaderProgram;

unsigned int shaderAttribute;

  
float data[3][3] = {
 {  0.0, 1.0, 0.0   },
 { -1.0, -1.0, 0.0  },
 {  1.0, -1.0, 0.0  }
};

GLuint texid;
GLuint texwidth=2;
GLuint texheight=2;
// GLuint texdepth=1;
GLubyte texData[16] =
{
	0xFF,0x00,0x00,0xFF, // red 
	0x00,0xFF,0x00,0xFF, // rgreen
	0x00,0x00,0xFF,0xFF, // blue
	0xFF,0xFF,0xFF,0xFF  // black 
};
 
void InitTexture() {
 
   glEnable(GL_TEXTURE_2D);
   glGenTextures(1,&texid);
   glBindTexture(GL_TEXTURE_2D,texid);
   glTexImage2D (
	GL_TEXTURE_2D,
	0,
	4,
	texwidth,
	texheight,
	0,
	GL_RGBA,
	GL_UNSIGNED_BYTE,
	texData
   ); 
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void main (int argc, char* argv[]) {
  glutInit(&argc, argv);
  glutCreateWindow("GLEW Test");
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    return;
  }
  fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
 
  shaderProgram = loadShader("vertex_shader.glsl", "fragment_shader.glsl");
  if (!shaderProgram) return;
  glUseProgram(shaderProgram->program);

  shaderAttribute = glGetAttribLocation(shaderProgram->program, "in_Position");
  fprintf(stdout, "shaderAttribute = %i\n", shaderAttribute);

  
  glGenBuffers(1, &triangleVBO);
  glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
  glVertexAttribPointer(shaderAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(shaderAttribute);

  glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
  
  glClearColor(0.0, 0.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  InitTexture();
  GLint samplerLoc = glGetUniformLocation(shaderProgram->program, "myTextureSampler");

  glUniform1i(samplerLoc, 0);
  glActiveTexture(GL_TEXTURE0 + 0);
  glBindTexture(GL_TEXTURE_2D, texid);
  glBindSampler(0, 0);
   
  while (1) {
   usleep(100);

  glDrawArrays(GL_TRIANGLES, 0, (sizeof(data) / 3) / sizeof(GLfloat));
  glFlush();
 }
}
