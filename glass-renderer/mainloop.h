#ifndef MAINLOOP_H
#define MAINLOOP_H

#include "xapi.h"
#include "list.h"
#include <sys/time.h>

#define event_mask_unset(member) memset(&member, 0x00, sizeof(member));
#define event_mask_set(member) memset(&member, 0xff, sizeof(member));

typedef struct FdEventHandlerStruct FdEventHandler;
typedef void FdEventHandlerFunction(FdEventHandler *handler, short revents);
struct FdEventHandlerStruct {
  int fd;
  short events;
  FdEventHandlerFunction *handler;
  void *data;
};

extern void mainloop_install_fd_event_handler(FdEventHandler *handler);
extern void mainloop_uninstall_fd_event_handler(FdEventHandler *handler);

typedef struct TimeoutHandlerStruct TimeoutHandler;
typedef void TimeoutHandlerFunction(TimeoutHandler *handler, struct timeval *current_time);
struct TimeoutHandlerStruct {
  struct timeval next;
  struct timeval interval;
 
  TimeoutHandlerFunction *handler;
  void *data;
};

extern void mainloop_install_timeout_handler(TimeoutHandler *handler);
extern void mainloop_uninstall_timeout_handler(TimeoutHandler *handler);

typedef struct {
  Display *display;
  FdEventHandler fd_handler;
  List *event_handlers;
} DisplayHandler;

extern DisplayHandler *mainloop_install_display(Display *display);
extern void mainloop_uninstall_display(DisplayHandler *handler);

typedef struct XEventHandlerStruct XEventHandler;
typedef Bool XEventHandlerFunction(XEventHandler *handler, XEvent *event);
struct XEventHandlerStruct {
  DisplayHandler *display;
 
  long event_mask;
  XEvent match_event;
  XEvent match_mask;
 
  XEventHandlerFunction *handler;
  void *data;
};

extern void mainloop_install_xevent_handler(XEventHandler *handler);
extern void mainloop_uninstall_xevent_handler(XEventHandler *handler);


//extern Bool mainloop_event_handle(XEvent *event);
extern void mainloop_run();
extern void mainloop_exit();


#endif
