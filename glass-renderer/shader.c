#include <GL/glew.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shader.h"
#include "glapi.h"
#include "xapi.h"
#include "wm.h"
#include "debug.h"
#include <math.h>
#include <string.h>

int shaderErrorLine(char *error) {
  char *c = index(error, ':');
  if (!c) return -1;
  c++;
  if (!*c) return -1;
  return atoi(c);
}
int shaderErrorCol(char *error) {
  char *c = index(error, '(');
  if (!c) return -1;
  c++;
  if (!*c) return -1;
  return atoi(c);
}
const char *indexn(const char *s, int c, int n) {
  for (; s && (n > 0); n--, s++)
    s = index(s, c);
  return s;
}
char *shaderErrorAnnotate(char *error, char *src) {
  int line = shaderErrorLine(error);
  int col = shaderErrorCol(error);
  
  char *res;
  
  if (line == -1 || col == -1) {
    return NULL;
  } else {
    res = malloc(strlen(src) + 1 + col + 1);
    const char *nextline = indexn(src, '\n', line);
    if (!nextline) { free(res); return NULL; };
    size_t nextlineidx = (nextline - src) + 1; // +1 to skip the newline
    
    strncpy(res, src, nextlineidx);
    if (col > 1) memset(res + nextlineidx, '-', col - 1);
    res[nextlineidx + col - 1] = '^';
    res[nextlineidx + col] = '\n';
    strcpy(res + nextlineidx + col + 1, src + nextlineidx);
    
    return res;
  }
}

int checkShaderError(char *name, char *part, char *src, GLuint shader) {
  GLint res;
  GLint len;
  GLchar *log;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
  if (res == GL_TRUE) return 1;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
  log = malloc(len + 1);
  glGetShaderInfoLog(shader, len, &len, log);

  char *annotated = shaderErrorAnnotate(log, src);

  ERROR("shader", "%s.%s shader compilation failed: %s [%d]\n\n%s\n\n", name, part, log, len, annotated ? annotated : src);
  if (annotated) free(annotated);
  
  GL_CHECK_ERROR("shader", "%s.%s", name, part);
  return 0;
}

int checkProgramError(char *name, GLuint program) {
  GLint res;
  GLint len;
  GLchar *log;
  glGetProgramiv(program, GL_LINK_STATUS, &res);
  if (res == GL_TRUE) return 1;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
  log = malloc(len + 1);
  glGetProgramInfoLog(program, len, &len, log);
  ERROR("shader", "%s shader linkage failed: %s [%d]\n", name, log, len);
  GL_CHECK_ERROR("checkProgramError", "%s", name);
  return 0;
}

Bool init_shader() {
  return True;
}

char *atom_load_string(Atom name) {
  if (!root_item) return NULL;
  Property *prop = properties_find(root_item->properties, name);
  if (!prop || prop->type == None) return NULL;
  char *res = malloc(prop->nitems + 1);
  memcpy(res, prop->values.bytes, prop->nitems);
  res[prop->nitems] = 0;
  return res;
}

Shader *shader_load(Atom name) {
  Shader *shader = malloc(sizeof(Shader));
  shader->geometry_src = NULL;
  shader->vertex_src = NULL;
  shader->fragment_src = NULL;
  shader->uniforms = NULL;
  
  do {
    shader->name = name;
    shader->name_str = XGetAtomName(display, name);

    shader->geometry = atom_append(display, shader->name, "_GEOMETRY");
    shader->vertex = atom_append(display, shader->name, "_VERTEX");
    shader->fragment = atom_append(display, shader->name, "_FRAGMENT");

    shader->geometry_src = atom_load_string(shader->geometry);
    if (!shader->geometry_src) { DEBUG("shader_load_geometry", "Failed\n"); break; }
    shader->geometry_shader = glCreateShader(GL_GEOMETRY_SHADER_ARB);
    glShaderSource(shader->geometry_shader, 1, (const GLchar**)&(shader->geometry_src), 0);
    glCompileShader(shader->geometry_shader);
    if (!checkShaderError(shader->name_str, "geometry", shader->geometry_src, shader->geometry_shader)) { DEBUG("shader_compile_geometry", "Failed\n"); break; }

    shader->vertex_src = atom_load_string(shader->vertex);
    if (!shader->vertex_src) { DEBUG("shader_load_vertex", "Failed\n"); break; }
    shader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(shader->vertex_shader, 1, (const GLchar**)&(shader->vertex_src), 0);
    glCompileShader(shader->vertex_shader);
    if (!checkShaderError(shader->name_str, "vertex", shader->vertex_src, shader->vertex_shader)) { DEBUG("shader_compile_vertex", "Failed\n"); break; }

    shader->fragment_src = atom_load_string(shader->fragment);
    if (!shader->fragment_src) { DEBUG("shader_load_fragment", "Failed\n"); break; }
    shader->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader->fragment_shader, 1, (const GLchar**)&(shader->fragment_src), 0);
    glCompileShader(shader->fragment_shader);
    if (!checkShaderError(shader->name_str, "fragment", shader->fragment_src, shader->fragment_shader)) { DEBUG("shader_compile_fragment", "Failed\n"); break; }

    shader->program = glCreateProgram();

    glAttachShader(shader->program, shader->geometry_shader);
    glAttachShader(shader->program, shader->vertex_shader);
    glAttachShader(shader->program, shader->fragment_shader);

    glLinkProgram(shader->program);
    if (!checkProgramError(shader->name_str, shader->program)) { DEBUG("shader_link", "Failed\n"); break; }

    glUseProgram(shader->program);

    GL_CHECK_ERROR("standard_properties1", "%s", shader->name_str);
    shader->screen_attr = glGetUniformLocation(shader->program, "screen");
    shader->size_attr = glGetUniformLocation(shader->program, "size");
    shader->border_width_attr = glGetUniformLocation(shader->program, "border_width");
    shader->picking_mode_attr = glGetUniformLocation(shader->program, "picking_mode");
    shader->window_id_attr = glGetUniformLocation(shader->program, "window_id");
    shader->widget_id_attr = glGetUniformLocation(shader->program, "widget_id");
    shader->window_sampler_attr = glGetUniformLocation(shader->program, "window_sampler");
    shader->pointer_attr = glGetUniformLocation(shader->program, "pointer");
    GL_CHECK_ERROR("standard_properties2", "%s", shader->name_str);

    shader->uniforms = list_create();

    GLint active_uniforms_nr;
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORMS, &active_uniforms_nr);
    for (int i = 0; i < active_uniforms_nr; i++) {
      Uniform *uniform = malloc(sizeof(Uniform));
      GLsizei  length;
      glGetActiveUniform(shader->program, i, sizeof(uniform->uniform_name), &length, &uniform->size, &uniform->uniform_type, uniform->uniform_name);

      if (memcmp("atom_", uniform->uniform_name, strlen("atom_")) == 0) {
        uniform->type = UNIFORM_ATOM;
        uniform->base_name = uniform->uniform_name + strlen("atom_");
        glUniform1i(i, XInternAtom(display, uniform->base_name, False));
      } else {
        uniform->type = UNIFORM_PROPERTY;
        uniform->base_name = uniform->uniform_name;
      }
      list_append(shader->uniforms, (void *) uniform);
    }

    return shader;
  } while (0);
  if (shader->geometry_src) free(shader->geometry_src);
  if (shader->vertex_src) free(shader->vertex_src);
  if (shader->fragment_src) free(shader->fragment_src);
  free(shader);
  return NULL;
}


List *shader_load_all(void) {
  if (!root_item) return NULL;
  Property *prop = properties_find(root_item->properties, ATOM("IG_SHADERS"));
  if (!prop || prop->type == None) return NULL;

  List *res = list_create();
  
  for (int i=0; i < prop->nitems; i++) {
    Shader *shader = shader_load((Atom ) prop->values.dwords[i]);
    if (shader) {
      list_append(res, (void *) shader);
    }
  }

  if (DEBUG_ENABLED("shaders")) {
    DEBUG("shaders", "Shaders:\n");
    for (size_t idx = 0; idx < res->count; idx++) {
      shader_print((Shader *) res->entries[idx], 0, stderr);
    }
  }
  
  return res;
}

void shader_free(Shader *shader) {
  XFree(shader->name_str);
  free(shader->geometry_src);
  free(shader->vertex_src);
  free(shader->fragment_src);
  for (size_t idx = 0; idx < shader->uniforms->count; idx++) {
    free(shader->uniforms->entries[idx]);
  }
  list_destroy(shader->uniforms);
  free(shader);
}

void shader_free_all(List *shaders) {
  if (!shaders) return;
  for (size_t idx = 0; idx < shaders->count; idx++) {
   shader_free((Shader *) shaders->entries[idx]);
  }
  list_destroy(shaders);
}

Shader *shader_find(List *shaders, Atom name) {
  if (!shaders) return NULL;
  for (size_t idx = 0; idx < shaders->count; idx++) {
    Shader *s = (Shader *) shaders->entries[idx];   
    if (s->name == name) {
      return s;
    }
  }
  return NULL;
}

void shader_print(Shader *shader, int indent, FILE *fp) {
  char *indentstr = get_indent(indent);
  fprintf(fp, "%s%s:\n", indentstr, XGetAtomName(display, shader->name));
  fprintf(fp, "%s  geometry: |\n", indentstr);
  indent_print(shader->geometry_src, indent + 4, fp);
  fprintf(fp, "%s  vertex: |\n", indentstr);
  indent_print(shader->vertex_src, indent + 4, fp);
  fprintf(fp, "%s  fragment: |\n", indentstr);
  indent_print(shader->fragment_src, indent + 4, fp);
}

void shader_reset_uniforms(Shader *shader) {
  const float f = nanf("initial");
  const float fm[16] = {f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f};
  for (size_t i = 0; i < shader->uniforms->count; i++) {
    Uniform *uniform = (Uniform *) shader->uniforms->entries[i];
    if (uniform->type != UNIFORM_PROPERTY) continue;
    
    GL_CHECK_ERROR("reset_uniforms1", "%s", shader->name_str);
    switch(uniform->uniform_type) {
      case GL_FLOAT: glUniform1f(i, f); break;
      case GL_FLOAT_VEC2: glUniform2f(i, f, f);  break;
      case GL_FLOAT_VEC3: glUniform3f(i, f, f, f);  break;
      case GL_FLOAT_VEC4: glUniform4f(i, f, f, f, f);  break;
      case GL_INT: glUniform1i(i, 0); break;
      case GL_INT_VEC2: glUniform2i(i, 0, 0); break;
      case GL_INT_VEC3: glUniform3i(i, 0, 0, 0); break;
      case GL_INT_VEC4: glUniform4i(i, 0, 0, 0, 0); break;
      case GL_BOOL: glUniform1i(i, 0); break;
      case GL_BOOL_VEC2: glUniform2i(i, 0, 0); break;
      case GL_BOOL_VEC3: glUniform3i(i, 0, 0, 0); break;
      case GL_BOOL_VEC4: glUniform4i(i, 0, 0, 0, 0); break;
      case GL_FLOAT_MAT2: glUniformMatrix2fv(i, 1, False, fm); break;
      case GL_FLOAT_MAT3: glUniformMatrix3fv(i, 1, False, fm); break;
      case GL_FLOAT_MAT4: glUniformMatrix4fv(i, 1, False, fm); break;
      case GL_SAMPLER_2D: glUniform1i(i, 0); break;
      case GL_SAMPLER_CUBE: glUniform1i(i, 0); break;
    }
    GL_CHECK_ERROR(uniform->uniform_name, "%s", shader->name_str);
  }
}
