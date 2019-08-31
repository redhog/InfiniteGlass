#include "event.h"
#include "list.h"
#include <string.h>

List *event_handlers = NULL;

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
  for (int idx = 0; idx < event_handlers->count; idx++) {
    EventHandler *handler = (EventHandler *) event_handlers->entries[idx];
    if (   event_match(event, &handler->match_event, &handler->match_mask)
        && handler->handler(handler, event)) {
      return True;
    }
  }
  return False;
}

void event_handler_install(EventHandler *handler) {
  if (!event_handlers) event_handlers = list_create();
  list_append(event_handlers, (void *) handler);
  // Fetch existing mask and OR!!!
  if (handler->match_mask.xany.window) {
    XSelectInput(display, handler->match_event.xany.window, handler->event_mask);
  }
}

void event_handler_uninstall(EventHandler *handler) {
  list_remove(event_handlers, (void *) handler);
}

