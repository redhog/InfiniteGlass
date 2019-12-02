#include "event.h"
#include "list.h"
#include "debug.h"
#include <string.h>

List *event_handlers = NULL;
List *timeout_handlers = NULL;

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


void timeout_handler_install(TimeoutHandler *handler) {
  if (!timeout_handlers) timeout_handlers = list_create();
  list_append(timeout_handlers, (void *) handler);
}

void timeout_handler_uninstall(TimeoutHandler *handler) {
  list_remove(timeout_handlers, (void *) handler);
}



Bool event_handle(XEvent *event) {
  if (!event_handlers) return False;
  for (int idx = 0; idx < event_handlers->count; idx++) {
    EventHandler *handler = (EventHandler *) event_handlers->entries[idx];
    if (   event_match(event, &handler->match_event, &handler->match_mask)
        && handler->handler(handler, event)) {
      return True;
    }
  }
  return False;
}

void timeout_handle() {
  if (!timeout_handlers) return;
  struct timeval current_time;
  if (gettimeofday(&current_time, NULL) != 0) {
    ERROR("gettimeofday", "gettimeofday() returned error");
  }
  for (int idx = 0; idx < timeout_handlers->count; idx++) {
    TimeoutHandler *handler = (TimeoutHandler *) timeout_handlers->entries[idx];
    if (timerisset(&handler->next) && timercmp(&current_time, &handler->next, >=)) {
      handler->handler(handler, &current_time);
      if (timerisset(&handler->interval)) {
        struct timeval res;
        while (timercmp(&current_time, &handler->next, >=)) {
          timeradd(&handler->next, &handler->interval, &res);
          handler->next = res;
        }
      } else {
        timerclear(&handler->next);
      }
    }
  }
  return;
}

Bool exit_mainloop_flag = False;

void event_mainloop() {
  int display_fd = ConnectionNumber(display);
  fd_set in_fds;
  struct timeval timeout;
  XEvent e;
    
  while (!exit_mainloop_flag) {
    XSync(display, False);
    
    FD_ZERO(&in_fds);
    FD_SET(display_fd, &in_fds);
    timeout.tv_usec = 30000;
    timeout.tv_sec = 0;

    int count = select(display_fd + 1, &in_fds, NULL, NULL, &timeout);

    if (count) {
      while (XPending(display)) {
        XNextEvent(display, &e);
        event_handle(&e);
      }
    }
      
    timeout_handle();
  }
}

void event_exit_mainloop() {
  exit_mainloop_flag = True;
}
