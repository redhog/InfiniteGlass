#include "view.h"
#include "xapi.h"
#include <limits.h>

void mat4mul(float *mat4, float *vec4, float *outvec4) {
  for (int i = 0; i < 4; i++) {
   float res = 0.0;
    for (int j = 0; j < 4; j++) {
      res += mat4[i*4 + j] * vec4[j];
    }
    outvec4[i] = res;
  }
  /*
  printf("|%f,%f,%f,%f||%f|   |%f|\n"
         "|%f,%f,%f,%f||%f|   |%f|\n"
         "|%f,%f,%f,%f||%f| = |%f|\n"
         "|%f,%f,%f,%f||%f|   |%f|\n",
         mat4[0], mat4[1], mat4[2], mat4[3],  vec4[0], outvec4[0],
         mat4[4], mat4[5], mat4[6], mat4[7],  vec4[1], outvec4[1],
         mat4[8], mat4[9], mat4[10],mat4[11], vec4[2], outvec4[2],
         mat4[12],mat4[13],mat4[14],mat4[15], vec4[3], outvec4[3]);
  */
}

void view_to_space(View *view, float screenx, float screeny, float *spacex, float *spacey) {
  float screen2space[4*4] = {view->screen[2]/view->width,0,0,view->screen[0],
                             0,-view->screen[3]/view->height,0,view->screen[1],
                             0,0,1,0,
                             0,0,0,1};
  float space[4] = {screenx, screeny, 0., 1.};
  float outvec[4];
  mat4mul(screen2space, space, outvec);
  *spacex = outvec[0];
  *spacey = outvec[1];
}
void view_from_space(View *view, float spacex, float spacey, float *screenx, float *screeny) {
  float space2screen[4*4] = {view->width/view->screen[2], 0., 0., -view->width*view->screen[0]/view->screen[2],
                             0., -view->height/view->screen[3], 0., -view->height*view->screen[1]/view->screen[3],
                             0., 0., 1., 0.,
                             0., 0., 0., 1.};
  float space[4] = {spacex, spacey, 0., 1.};
  float outvec[4];
  mat4mul(space2screen, space, outvec);
  *screenx = outvec[0];
  *screeny = outvec[1];
}




void view_abstract_draw(View *view, Item **items, ItemFilter *filter) {
  for (; items && *items; items++) {
    if (!filter || filter(*items)) {
      (*items)->type->draw(view, *items);
    }
  }
}

void view_draw(GLint fb, View *view, Item **items, ItemFilter *filter) {
  gl_check_error("draw0");
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glEnablei(GL_BLEND, 0);
  glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
  gl_check_error("draw1");
  view->picking = 0;
  view_abstract_draw(view, items, filter);
  gl_check_error("draw2");
  gl_check_error("draw3");
}

void view_pick(GLint fb, View *view, Item **items, ItemFilter *filter, int x, int y, int *winx, int *winy, Item **returnitem) {
  float data[4];
  memset(data, 0, sizeof(data));
  gl_check_error("pick1");
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  view->picking = 1;
  view_abstract_draw(view, items, filter);
  glReadPixels(x, view->height - y, 1, 1, GL_RGBA, GL_FLOAT, (GLvoid *) data);
  gl_check_error("pick2");
  //fprintf(stderr, "Pick %d,%d -> %f,%f,%f,%f\n", x, y, data[0], data[1], data[2], data[3]);
  *winx = 0;
  *winy = 0;
  *returnitem = NULL;
  if (data[2] != 0.0) {
    *returnitem = item_get(data[2] * (float) INT_MAX);
    if (*returnitem) {
      *winx = (int) (data[0] * (*returnitem)->width);
      *winy = (int) (data[1] * (*returnitem)->height);
    }
  }
  /*
  if (*returnitem) {
    fprintf(stderr, "Pick %d,%d -> %d,%d,%d\n", x, y, (*returnitem)->window, *winx, *winy);
  } else {
    fprintf(stderr, "Pick %d,%d -> NULL\n", x, y);
  }
  */
}
