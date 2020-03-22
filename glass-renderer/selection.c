#include "xapi.h"
#include "mainloop.h"
#include "selection.h"
#include "debug.h"

Bool init_selection(void) {
  return True;
}

int selection_get_params(Selection *selection, XEvent *event, long offset, long length,
                         Atom *actual_type_return, int *actual_format_return,
                         unsigned long *nitems_return, unsigned long *bytes_after_return, unsigned char **prop_return) {
 return XGetWindowProperty(selection->mainloop->conn->display, event->xselectionrequest.requestor, event->xselectionrequest.property,
                           offset, length,
                           0, AnyPropertyType,
                           actual_type_return, actual_format_return, nitems_return, bytes_after_return, prop_return);
} 

void selection_answer(Selection *selection, XEvent *event, Atom type, int format, int mode, unsigned char *data, int nelements) {
  XChangeProperty(selection->mainloop->conn->display, event->xselectionrequest.requestor, event->xselectionrequest.property,
                  type, format, mode, data, nelements);
}

Bool event_handler_convert(EventHandler *handler, XEvent *event) {
  Selection *selection = (Selection *) handler->data;
  Bool status = selection->handler(handler->data, event);
   
  XSelectionEvent reply;
  reply.type = SelectionNotify;
  reply.requestor = event->xselectionrequest.requestor;
  reply.selection = event->xselectionrequest.selection;
  reply.target = event->xselectionrequest.target;
  reply.time = event->xselectionrequest.time;

  if (status) {
    reply.property = event->xselectionrequest.property;
  } else {
    reply.property = None;
  }
  XSendEvent(selection->mainloop->conn->display, reply.requestor, False, NoEventMask, (XEvent *) &reply);
  return True;
}

Bool event_handler_clear(EventHandler *handler, XEvent *event) {
  Selection *selection = (Selection *) handler->data;
  selection->clear(selection);

  mainloop_uninstall_event_handler(&selection->event_handler_convert);
  mainloop_uninstall_event_handler(&selection->event_handler_clear);
  free(selection);
   
  return True;
}

void selection_destroy(Selection *selection) {
  // The actual selection object is destroyed by the clear event handler.
  if (selection->ownsowner) {
    XDestroyWindow(selection->mainloop->conn->display, selection->owner);
  } else {
    XSetSelectionOwner(selection->mainloop->conn->display, selection->name, None, CurrentTime);
  }
}

Selection *selection_create(Mainloop *mainloop, Window owner, Atom name, SelectionHandler *handler, SelectionClearHandler *clear, void *data) {
  Selection *selection = malloc(sizeof(Selection));

  selection->mainloop = mainloop;
  selection->owner = owner;
  selection->ownsowner = False;
  selection->name = name;
  selection->handler = handler;
  selection->clear = clear;
  selection->data = data;
  
  if (selection->owner == None) {
    selection->owner = XCreateSimpleWindow(mainloop->conn->display, mainloop->conn->root, -1, -1, 1, 1, 0, 0, 0);
    selection->ownsowner = True;
  }

  EventHandler *event_handler;

  event_handler = &selection->event_handler_convert;
  event_handler->mainloop = mainloop;
  event_handler->event_mask = NoEventMask;
  event_mask_unset(event_handler->match_event);
  event_mask_unset(event_handler->match_mask);
  event_mask_set(event_handler->match_mask.xselectionrequest.type);
  event_mask_set(event_handler->match_mask.xselectionrequest.selection);
  event_handler->match_event.xselectionrequest.type = SelectionRequest;
  event_handler->match_event.xselectionrequest.selection = selection->name;
  event_handler->handler = &event_handler_convert;
  event_handler->data = selection;
  mainloop_install_event_handler(event_handler);

  event_handler = &selection->event_handler_clear;
  event_handler->mainloop = mainloop;
  event_handler->event_mask = NoEventMask;
  event_mask_unset(event_handler->match_event);
  event_mask_unset(event_handler->match_mask);
  event_mask_set(event_handler->match_mask.xselectionrequest.type);
  event_mask_set(event_handler->match_mask.xselectionrequest.selection);
  event_handler->match_event.xselectionrequest.type = SelectionClear;
  event_handler->match_event.xselectionrequest.selection = selection->name;
  event_handler->handler = &event_handler_clear;
  event_handler->data = selection;
  mainloop_install_event_handler(event_handler);
  
  // Generate timestamp
  char dummy;
  XEvent timestamp_event;
  XSelectInput(mainloop->conn->display, selection->owner, PropertyChangeMask);
  XChangeProperty(mainloop->conn->display, selection->owner, selection->name, XA_STRING, 8, PropModeAppend, (void *) &dummy, 0);
  DEBUG("selection.create", "Waiting for timestamp\n");
  XWindowEvent(mainloop->conn->display, selection->owner, PropertyChangeMask, &timestamp_event);

  XSetSelectionOwner(mainloop->conn->display, selection->name, selection->owner, timestamp_event.xproperty.time);
  Window current_owner = XGetSelectionOwner(mainloop->conn->display, selection->name);
  if (current_owner != selection->owner) {
    mainloop_uninstall_event_handler(&selection->event_handler_convert);
    mainloop_uninstall_event_handler(&selection->event_handler_clear);
    free(selection);
    return NULL;
  } else {
    selection->time = timestamp_event.xproperty.time;
    return selection;
  }
}

Selection *manager_selection_create(Mainloop *mainloop, Atom name, SelectionHandler *handler, SelectionClearHandler *clear, void *data, Bool force, long arg1, long arg2) {
  Window existing = XGetSelectionOwner(mainloop->conn->display, name);
  
  if (existing != None) {
    if (!force) {
      return NULL;
    }
    XSelectInput(mainloop->conn->display, existing, StructureNotifyMask);
  }
  
  Selection *selection = selection_create(mainloop, None, name, handler, clear, data);
  
  if (existing != None) {
    XEvent destroy_event;
    while (1) {
      // FIXME: Timeout for this and use XKillClient if it times out...
      DEBUG("selection.create.manager", "Waiting for existing window to destroy\n");
      XWindowEvent(mainloop->conn->display, existing, StructureNotifyMask, &destroy_event);
      if (destroy_event.type == DestroyNotify) break;
    }
  }

  XClientMessageEvent event;
  event.type = ClientMessage;
  event.message_type = ATOM(mainloop->conn, "XA_MANAGER");
  event.format = 32;
  event.window = mainloop->conn->root;
  event.data.l[0] = selection->time;
  event.data.l[1] = selection->name;
  event.data.l[2] = selection->owner;
  event.data.l[3] = arg1;
  event.data.l[4] = arg2;
  XSendEvent(mainloop->conn->display, mainloop->conn->root, False, StructureNotifyMask, (XEvent *) &event);
  
  return selection;
}
