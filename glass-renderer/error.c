#include "error.h"
#include "list.h"
#include <unistd.h>

#define MAX_ERROR_TEXT_LENGTH 1024

List *error_handlers = NULL;

void throw(XErrorEvent *error) {
  for (int i = error_handlers->count - 1; i > 0; i--) {
    ErrorHandler *handler = (ErrorHandler *) error_handlers->entries[i];
    if (handler->handler && handler->handler(handler, error)) {
      break;
    }
  }
}

int x_error_handler(Display* display, XErrorEvent* e) {
  throw(e);
  return 0;
}


void error_handler_push(ErrorHandler *handler) {
  list_append(error_handlers, (void *) handler);
}

ErrorHandler *error_handler_pop() {
  return (ErrorHandler *) list_pop(error_handlers);
}

void print_error(XErrorEvent *event) {
  char error_text[MAX_ERROR_TEXT_LENGTH];
  XGetErrorText(display, event->error_code, error_text, sizeof(error_text));
  printf("Error:\n");
  for (int i = 0; i < error_handlers->count; i++) {
    ErrorHandler *handler = (ErrorHandler *) error_handlers->entries[i];
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
  print_error(event);
  if (getenv("GLASS_ERROR_EXIT") != NULL || fork() == 0) {
    *((char *) 0) = 0;
  }
  return 1;
}

ErrorHandler error_handler_base = {&error_handler_base_function, NULL, "Base error context"};


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

void try() {
  ErrorHandler *handler = malloc(sizeof(ErrorHandler));
  handler->handler = &error_handler_try;
  handler->data = malloc(sizeof(ErrorTry));
  ((ErrorTry *) handler->data)->has_error = False;
  handler->context = NULL;
  error_handler_push(handler);
}

int catch(XErrorEvent *error) {
  ErrorHandler *handler = error_handler_pop();
  ErrorTry *try = (ErrorTry *) handler->data;
  int res = try->has_error;
  *error = try->event;
  free(try);
  free(handler);
  return !res;
}

void error_init() {
  error_handlers = list_create();
  error_handler_push(&error_handler_base);
  XSetErrorHandler(&x_error_handler);
}
