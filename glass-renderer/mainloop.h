#ifndef MAINLOOP_H
#define MAINLOOP_H

#include "xapi.h"
#include <sys/time.h>

#define event_mask_unset(member) memset(&member, 0x00, sizeof(member));
#define event_mask_set(member) memset(&member, 0xff, sizeof(member));

typedef struct EventHandlerStruct EventHandler;
typedef Bool EventHandlerFunction(EventHandler *handler, XEvent *event);
struct EventHandlerStruct {
  long event_mask;
  XEvent match_event;
  XEvent match_mask;
 
  EventHandlerFunction *handler;
  void *data;
};

extern void mainloop_install_event_handler(EventHandler *handler);
extern void mainloop_uninstall_event_handler(EventHandler *handler);

typedef struct TimeoutHandlerStruct TimeoutHandler;
typedef void TimeoutHandlerFunction(TimeoutHandler *handler, struct timeval *current_time);
struct TimeoutHandlerStruct {
  struct timeval next;
  struct timeval interval;
 
  TimeoutHandlerFunction *handler;
  void *data;
};

typedef void XCBCookieHandlerFunction(void *data, void *reply, xcb_generic_error_t *error); 

extern void mainloop_install_timeout_handler(TimeoutHandler *handler);
extern void mainloop_uninstall_timeout_handler(TimeoutHandler *handler);
extern void mainloop_install_xcb_cookie_handler(unsigned int request, XCBCookieHandlerFunction *fn, void *data);
#define MAINLOOP_XCB_DEFER(cookie, handler, data) mainloop_install_xcb_cookie_handler(cookie.sequence, (XCBCookieHandlerFunction *) handler, data)

extern Bool mainloop_event_handle(XEvent *event);
extern Bool mainloop_run();
extern void mainloop_exit();


#endif
