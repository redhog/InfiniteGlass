#include "input.h"

InputMode **input_mode_stack = NULL;
size_t input_mode_stack_len = 0;
size_t input_mode_stack_size = 10;

uint input_mode_stack_handle(XEvent event) {
  for (ssize_t idx = input_mode_stack_len - 1; idx >= 0; idx--) {
    if (input_mode_stack[idx]->handle_event(idx, event)) {
      return True;
    }
  }
  return False;
}

void push_input_mode(InputMode *mode) {
  input_mode_stack_len++;
  if (input_mode_stack_len > input_mode_stack_size || !input_mode_stack) {
    input_mode_stack = (InputMode **) realloc(input_mode_stack, sizeof(InputMode*) * input_mode_stack_size);
  }
  input_mode_stack[input_mode_stack_len-1] = mode;
  input_mode_stack[input_mode_stack_len] = NULL;
}

InputMode *pop_input_mode() {
  InputMode *mode = input_mode_stack[input_mode_stack_len-1];
  input_mode_stack[input_mode_stack_len-1] = NULL;
  input_mode_stack_len--;
  return mode;
}



uint base_input_mode_handle_event(size_t mode, XEvent event) {
  return 0;
};

BaseInputMode base_input_mode = {
  {
    base_input_mode_handle_event
  }
};
ZoomPanInputMode zoom_pan_input_mode = {
};
ItemInputMode item_input_mode = {
};
