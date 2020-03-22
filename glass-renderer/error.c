#include "error.h"
#include "list.h"
#include <unistd.h>

#define MAX_ERROR_TEXT_LENGTH 1024

void throw(XConnection *conn, XErrorEvent *error) {
  for (int i = conn->error_handlers->count - 1; i > 0; i--) {
    ErrorHandler *handler = (ErrorHandler *) conn->error_handlers->entries[i];
    if (handler->handler && handler->handler(handler, error)) {
      break;
    }
  }
}

int x_error_handler(Display* display, XErrorEvent* e) {
  XConnection *conn = NULL;
 
  XFindContext(display, DefaultRootWindow(display), x_conn_context, (XPointer *) &conn);
  throw(conn, e);
  return 0;
}


void error_handler_push(ErrorHandler *handler) {
  list_append(handler->conn->error_handlers, (void *) handler);
}

ErrorHandler *error_handler_pop(XConnection *conn) {
  return (ErrorHandler *) list_pop(conn->error_handlers);
}

void print_error(XConnection *conn, XErrorEvent *event) {
  char error_text[MAX_ERROR_TEXT_LENGTH];
  XGetErrorText(conn->display, event->error_code, error_text, sizeof(error_text));
  printf("Error:\n");
  for (int i = 0; i < conn->error_handlers->count; i++) {
    ErrorHandler *handler = (ErrorHandler *) conn->error_handlers->entries[i];
    if (handler->context) {
      printf("  %s\n", handler->context);
    }
  }
  fprintf(stderr,
          "  %s (%i): request: %i, resource: %u\n",
          error_text,
          event->error_code,
          event->request_code,
          (uint) event->resourceid);
}

int error_handler_base_function(ErrorHandler *handler, XErrorEvent *event) {
  print_error(handler->conn, event);
  if (getenv("GLASS_ERROR_EXIT") != NULL || fork() == 0) {
    *((char *) 0) = 0;
  }
  return 1;
}

typedef struct {
  int has_error;
  XErrorEvent event;
} ErrorTry;

int error_handler_try(ErrorHandler *handler, XErrorEvent *event) {
  ErrorTry *data = (ErrorTry*) handler->data;
  data->has_error = True;
  data->event = *event;
  return 1;
}

void try(XConnection *conn) {
  ErrorHandler *handler = malloc(sizeof(ErrorHandler));
  handler->conn = conn;
  handler->handler = &error_handler_try;
  handler->data = malloc(sizeof(ErrorTry));
  ((ErrorTry *) handler->data)->has_error = False;
  handler->context = NULL;
  error_handler_push(handler);
}

int catch(XConnection *conn, XErrorEvent *error) {
  ErrorHandler *handler = error_handler_pop(conn);
  ErrorTry *try = (ErrorTry *) handler->data;
  int res = try->has_error;
  *error = try->event;
  free(try);
  free(handler);
  return !res;
}

void error_init(XConnection *conn) {
  conn->error_handlers = list_create();
  ErrorHandler *error_handler = malloc(sizeof(ErrorHandler));
  error_handler->conn = conn;
  error_handler->handler = &error_handler_base_function;
  error_handler->data = NULL;
  error_handler->context = "Base error context";
  error_handler_push(error_handler);
  XSetErrorHandler(&x_error_handler);
}
