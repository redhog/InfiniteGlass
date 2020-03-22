#include "mainloop.h"
#include "list.h"
#include "debug.h"
#include <string.h>

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

void mainloop_install_event_handler(EventHandler *handler) {
  if (!handler->mainloop->mainloop_event_handlers) handler->mainloop->mainloop_event_handlers = list_create();
  list_append(handler->mainloop->mainloop_event_handlers, (void *) handler);
  // Fetch existing mask and OR!!!
  if (handler->match_mask.xany.window) {
    XSelectInput(handler->mainloop->conn->display, handler->match_event.xany.window, handler->event_mask);
  }
}

void mainloop_uninstall_event_handler(EventHandler *handler) {
  list_remove(handler->mainloop->mainloop_event_handlers, (void *) handler);
}


void mainloop_install_timeout_handler(TimeoutHandler *handler) {
  if (!handler->mainloop->timeout_handlers) handler->mainloop->timeout_handlers = list_create();
  list_append(handler->mainloop->timeout_handlers, (void *) handler);
}

void mainloop_uninstall_timeout_handler(TimeoutHandler *handler) {
  list_remove(handler->mainloop->timeout_handlers, (void *) handler);
}



Bool mainloop_event_handle(Mainloop *mainloop, XEvent *event) {
  if (!mainloop->mainloop_event_handlers) return False;
  for (int idx = 0; idx < mainloop->mainloop_event_handlers->count; idx++) {
    EventHandler *handler = (EventHandler *) mainloop->mainloop_event_handlers->entries[idx];
    if (   event_match(event, &handler->match_event, &handler->match_mask)
        && handler->handler(handler, event)) {
      return True;
    }
  }
  return False;
}

void timeout_handle(Mainloop *mainloop) {
  if (!mainloop->timeout_handlers) return;
  struct timeval current_time;
  if (gettimeofday(&current_time, NULL) != 0) {
    ERROR("gettimeofday", "gettimeofday() returned error");
  }
  for (int idx = 0; idx < mainloop->timeout_handlers->count; idx++) {
    TimeoutHandler *handler = (TimeoutHandler *) mainloop->timeout_handlers->entries[idx];
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

void mainloop_run(Mainloop *mainloop) {
  int display_fd = ConnectionNumber(mainloop->conn->display);
  fd_set in_fds;
  struct timeval timeout;
  XEvent e;
    
  while (!mainloop->exit_mainloop_flag) {
    FD_ZERO(&in_fds);
    FD_SET(display_fd, &in_fds);
    timeout.tv_usec = 30000;
    timeout.tv_sec = 0;

    select(display_fd + 1, &in_fds, NULL, NULL, &timeout);
    timeout_handle(mainloop);
    while (XPending(mainloop->conn->display)) {
      XNextEvent(mainloop->conn->display, &e);
      mainloop_event_handle(mainloop, &e);
      XSync(mainloop->conn->display, False);
    }
  }
}

void mainloop_exit(Mainloop *mainloop) {
  mainloop->exit_mainloop_flag = True;
}

Mainloop *mainloop_create(XConnection *conn) {
  Mainloop *mainloop = malloc(sizeof(Mainloop));
  mainloop->mainloop_event_handlers = NULL;
  mainloop->timeout_handlers = NULL;
  mainloop->exit_mainloop_flag = False;
  mainloop->conn = conn;
  return mainloop;
}

void mainloop_destroy(Mainloop *mainloop) {
  while (mainloop->mainloop_event_handlers->count) {
    EventHandler *handler = (EventHandler *) mainloop->mainloop_event_handlers->entries[0];
    mainloop_uninstall_event_handler(handler);
  }
  while (mainloop->timeout_handlers->count) {
    TimeoutHandler *handler = (TimeoutHandler *) mainloop->timeout_handlers->entries[0];
    mainloop_uninstall_timeout_handler(handler);
  }
  list_destroy(mainloop->mainloop_event_handlers);
  list_destroy(mainloop->timeout_handlers);
  free(mainloop);
}
