#include "event.h"
#include <string.h>

EventHandler **event_handlers;
size_t nr_event_handlers;
size_t event_handlers_size;

Bool event_match(XEvent *event, XEvent *match_event, XEvent *match_mask) {
 char *e = (char *) event;
 char *me = (char *) match_event;
 char *mm = (char *) match_mask;

  for (size_t idx = 0; idx < sizeof(XEvent); idx++) {
    if ((e[idx] & mm[idx]) != (me[idx] & mm[idx])) {
      return False;
    }
  }
  return True;
}

Bool event_handle(XEvent *event) {
  for (int idx = 0; idx < nr_event_handlers; idx++) {
    if (   event_match(event, &event_handlers[idx]->match_event, &event_handlers[idx]->match_mask)
        && event_handlers[idx]->handler(event_handlers[idx], event)) {
      return True;
    }
  }
  return False;
}

void event_handler_install(EventHandler *handler) {
  if (nr_event_handlers+1 >= event_handlers_size) {
    event_handlers_size = nr_event_handlers + 32;
    event_handlers = realloc(event_handlers, event_handlers_size * sizeof(EventHandler*));
  }
  event_handlers[nr_event_handlers] = handler;
  // Fetch existing mask and OR!!!
  if (handler->match_mask.xany.window) {
    XSelectInput(display, handler->match_event.xany.window, handler->event_mask);
  }
  nr_event_handlers++;
}

void event_handler_uninstall(EventHandler *handler) {
  size_t idx;
  for (idx = 0; idx < nr_event_handlers && event_handlers[idx] != handler; idx++);
  if (idx < nr_event_handlers) {
    memmove(event_handlers + idx, event_handlers + idx+1, nr_event_handlers - idx - 1);
    nr_event_handlers -= 1;
  }
}

