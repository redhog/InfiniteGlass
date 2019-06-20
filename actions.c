#include "actions.h"
#include "screen.h"
#include "xapi.h"

void action_zoom_screen_home() {
  screen[0] = 0.;
  screen[1] = 0.;
  screen[2] = 1.;
  screen[3] = (float) overlay_attr.height / (float) overlay_attr.width;
  draw();
}

void action_zoom_to_window(Item *item) {
  printf("%f,%f[%f,%f] - %f,%f[%f,%f]\n",
         screen[0],screen[1],screen[2],screen[3],
         item->coords[0],item->coords[1],item->coords[2],item->coords[3]);
  screen[2] = item->coords[2];
  screen[3] = item->coords[2] * (float) overlay_attr.height / (float) overlay_attr.width;
  screen[0] = item->coords[0];
  screen[1] = item->coords[1] - screen[3];
  draw();
}

void action_zoom_window_to_1_to_1_to_screen(Item *item) {
  item->width = overlay_attr.width * item->coords[2]/screen[2];
  item->height = overlay_attr.height * item->coords[3]/screen[3];
  item->type->update(item);
  draw();
}

void action_zoom_window_by(Item *item, float factor) {
  item->width = item->width * factor;
  item->height = item->height * factor;
  item->type->update(item);
  draw();
}

void action_zoom_screen_to_1_to_1_to_window(Item *item) {
  screen[2] = overlay_attr.width * item->coords[2]/item->width;
  screen[3] = overlay_attr.height * item->coords[3]/item->height;

  screen[0] = item->coords[0] - (screen[2] - item->coords[2]) / 2.;
  screen[1] = item->coords[1] - (screen[3] + item->coords[3]) / 2.;

  draw();
}

void action_zoom_screen_to_window_and_window_to_screen(Item *item) {
  printf("%f,%f[%f,%f] - %f,%f[%f,%f]\n",
         screen[0],screen[1],screen[2],screen[3],
         item->coords[0],item->coords[1],item->coords[2],item->coords[3]);
  
  item->width = overlay_attr.width;
  item->height = overlay_attr.height;
  item->coords[3] = overlay_attr.height * item->coords[2] / overlay_attr.width;

  screen[2] = item->coords[2];
  screen[3] = item->coords[3];
  screen[0] = item->coords[0];
  screen[1] = item->coords[1] - screen[3];

  item->type->update(item);
  
  draw();
}
