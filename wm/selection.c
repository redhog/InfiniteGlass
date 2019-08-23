#include "xapi.h"
#include "event.h"



int selection_get_params(Selection *selection, XEvent *event, long offset, long length,
                         Atom *actual_type_return, int *actual_format_return,
                         unsigned long *nitems_return, unsigned long *bytes_after_return, unsigned char **prop_return) {
 return XGetWindowProperty(display, event->xselectionrequest.requestor, event->xselectionrequest.property,
                           offset, length,
                           0, AnyPropertyType,
                           actual_type_return, actual_format_return, nitems_return, bytes_after_return, prop_return);
} 

void selection_answer(Selection *selection, XEvent *event, Atom type, int format, int mode, unsignedchar *data, int nelements) {
  XChangeProperty(display, event->xselectionrequest.requestor, event->xselectionrequest.property,
                  type, format, mode, data, nelements);
}

Bool event_handler_convert(EventHandler *handler, XEvent *event) {
  Bool status = ((Selection *) handler->data)->handler(handler->data, event)
   
  XSelectionEvent reply;
  reply.type = SelectionNotify;
  reply.requestor = event.xselectionrequest.requestor;
  reply.selection = event.xselectionrequest.selection;
  reply.target = event.xselectionrequest.target;
  reply.time = event.xselectionrequest.time;

  if (status) {
    reply.property = event.xselectionrequest.property;
  } else {
    reply.property = None;
  }
  XSendEvent(display, reply.requestor, False, NoEventMask, &reply);
}

Select *selection_create(Window owner, Atom name, SelectionHandler *handler, void *data) {
  Selection *selection = malloc(sizeof(Selection));

  selection->owner = owner;
  selection->name = name;
  selection->handler = handler;
  selection->data = data;
  
  if (selection->owner == None) {
    selection->owner = XCreateSimpleWindow(display, root, -1, -1, 1, 1, 0, 0, 0);
  }

  EventHandler *handler;

  handler = &selection->event_handler_convert;
  handler->event_mask = NoEventMask;
  event_mask_unset(&handler->match_event);
  event_mask_unset(&handler->match_mask);
  event_mask_set(&handler->match_mask.xselectionrequest.type);
  event_mask_set(&handler->match_mask.xselectionrequest.selection);
  handler->match_event.xselectionrequest.type = SelectionRequest
  handler->match_event.xselectionrequest.selection = selection->name;
  handler->handler = &event_handler_convert;
  handler->data = selection;
  event_handler_install(handler);


  // Generate timestamp
  char data;
  XEvent timestamp_event;
  XChangeProperty(display, selection->owner, selection->name, XA_STRING, 8, PropModeAppend, &data, 0);
  XWindowEvent(display, selection->owner, PropertyChangeMask, &timestamp_event);

  XSetSelectionOwner(display, selection->name, selection->owner, timestamp_event->xproperty.time);
  Window owner = XGetSelectionOwner(display, selection->name);
  if (owner != selection->owner) {
    return Fallse;
  } else {
    selection->time = timestamp_event->xproperty.time;
    return True;
  }
}

Select *manager_selection_create(Atom name, SelectionHandler *handler, void *data, Bool force, long arg1, long arg2) {
  ManagerSelection *selection = malloc(sizeof(ManagerSelection));
  Window existing = XGetSelectionOwner(display, name);
  
  if (existing != None) {
    if (!force) {
      return NULL;
    }
    XSelectInput(display, existing, StructureNotifyMask);
  }
  
  selection->selection = selection_create(None, name, handler, data);
  
  if (existing != None) {
    XEvent destroy_event;
    while (1) {
      // FIXME: Timeout for this and use XKillClient if it times out...
      XWindowEvent(display, existing, StructureNotifyMask, &destroy_event);
      if (destroy_event.type == DestroyNotify) break;
    }
  }

  XClientMessageEvent event;
  event.type = ClientMessage;
  event.message_type = XA_MANAGER;
  event.format = 32;
  event.window = root;
  event.data.l[0] = selection->time;
  event.data.l[1] = selection->name;
  event.data.l[2] = selection->owner;
  event.data.l[3] = arg1;
  event.data.l[4] = arg2;
  XSendEvent(display, root, False, StructureNotify, &event);
  
  return selection;
}
