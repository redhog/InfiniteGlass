#ifndef ACTIONS
#define ACTIONS

#include "item.h"

extern void action_zoom_screen_home();
extern void action_zoom_to_window(Item *item);
extern void action_zoom_window_to_1_to_1_to_screen(Item *item);
extern void action_zoom_window_by(Item *item, float factor);
extern void action_zoom_screen_to_1_to_1_to_window(Item *item);
extern void action_zoom_screen_to_window_and_window_to_screen(Item *item);

#endif
