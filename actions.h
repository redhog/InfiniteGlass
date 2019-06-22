#ifndef ACTIONS
#define ACTIONS

#include "item.h"

extern void action_zoom_screen_home(View *view);
extern void action_zoom_to_window(View *view, Item *item);
extern void action_zoom_window_to_1_to_1_to_screen(View *view, Item *item);
extern void action_zoom_window_by(View *view, Item *item, float factor);
extern void action_zoom_screen_by(View *view, float factor);
extern void action_zoom_screen_by_around(View *view, float factor, int x, int y);
extern void action_zoom_screen_to_1_to_1_to_window(View *view, Item *item);
extern void action_zoom_screen_to_window_and_window_to_screen(View *view, Item *item);

#endif
