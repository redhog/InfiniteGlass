#include "actions.h"
#include "view.h"
#include "xapi.h"
#include "wm.h"

void action_zoom_screen_home(View *view) {
  view->screen[0] = 0.;
  view->screen[1] = 0.;
  view->screen[2] = 1.;
  view->screen[3] = (float) view->height / (float) view->width;
  draw();
}

void action_zoom_to_window(View *view, Item *item) {
  printf("%f,%f[%f,%f] - %f,%f[%f,%f]\n",
         view->screen[0],view->screen[1],view->screen[2],view->screen[3],
         item->coords[0],item->coords[1],item->coords[2],item->coords[3]);
  view->screen[2] = item->coords[2];
  view->screen[3] = item->coords[2] * (float) view->height / (float) view->width;
  view->screen[0] = item->coords[0];
  view->screen[1] = item->coords[1] - view->screen[3];
  draw();
}

void action_zoom_window_to_1_to_1_to_screen(View *view, Item *item) {
  item->width = view->width * item->coords[2]/view->screen[2];
  item->height = view->height * item->coords[3]/view->screen[3];
  item->type->update(item);
  draw();
}

void action_zoom_window_by(View *view, Item *item, float factor) {
  item->width = item->width * factor;
  item->height = item->height * factor;
  item->type->update(item);
  draw();
}

void action_zoom_screen_by(View *view, float factor) {
  float mousex = view->screen[0] + 0.5 * view->screen[2];
  float mousey = view->screen[1] + 0.5 * view->screen[3];

  view->screen[2] = view->screen[2] * factor;
  view->screen[3] = view->screen[3] * factor;

  view->screen[0] = mousex - 0.5 * view->screen[2];
  view->screen[1] = mousey - 0.5 * view->screen[3];

  draw();
}

void action_zoom_screen_by_around(View *view, float factor, int x, int y) {
  float spacex = ((float) x) / (float) view->width;
  float spacey = ((float) (view->height - y)) / (float) view->width;

  float mousex = view->screen[0] + spacex * view->screen[2];
  float mousey = view->screen[1] + spacey * view->screen[3];

  view->screen[2] = view->screen[2] * factor;
  view->screen[3] = view->screen[3] * factor;

  view->screen[0] = mousex - spacex * view->screen[2];
  view->screen[1] = mousey - spacey * view->screen[3];

  draw();
}

void action_zoom_screen_to_1_to_1_to_window(View *view, Item *item) {
  view->screen[2] = view->width * item->coords[2]/item->width;
  view->screen[3] = view->height * item->coords[3]/item->height;

  view->screen[0] = item->coords[0] - (view->screen[2] - item->coords[2]) / 2.;
  view->screen[1] = item->coords[1] - (view->screen[3] + item->coords[3]) / 2.;

  draw();
}

void action_zoom_screen_to_window_and_window_to_screen(View *view, Item *item) {
  printf("%f,%f[%f,%f] - %f,%f[%f,%f]\n",
         view->screen[0],view->screen[1],view->screen[2],view->screen[3],
         item->coords[0],item->coords[1],item->coords[2],item->coords[3]);
  
  item->width = view->width;
  item->height = view->height;
  item->coords[3] = view->height * item->coords[2] / view->width;

  view->screen[2] = item->coords[2];
  view->screen[3] = item->coords[3];
  view->screen[0] = item->coords[0];
  view->screen[1] = item->coords[1] - view->screen[3];

  item->type->update(item);
  
  draw();
}
