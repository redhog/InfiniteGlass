#ifndef INPUT
#define INPUT

#include "xapi.h"
#include "space.h"

typedef void EnterMode(size_t mode);
typedef void ExitMode(size_t mode);
typedef void ConfigureWindow(size_t mode, Window window);
typedef void UnconfigureWindow(size_t mode, Window window);
typedef uint HandleEvent(size_t mode, XEvent event);

typedef struct {
  EnterMode *enter;
  ExitMode *exit;
  ConfigureWindow *configure;
  UnconfigureWindow *unconfigure;
  HandleEvent *handle_event;
  XEvent first_event;
  XEvent last_event;
} InputMode;

InputMode **input_mode_stack;
size_t input_mode_stack_len;

void input_mode_stack_configure(Window window);
uint input_mode_stack_handle(XEvent event);
void push_input_mode(InputMode *mode);
InputMode *pop_input_mode();


typedef struct {
  InputMode base;
} BaseInputMode;
typedef struct {
  InputMode base;
} ZoomInputMode;
typedef struct {
  InputMode base;
  float screen_orig[4];
  int x;
  int y;
} PanInputMode;
typedef struct {
  InputMode base;
  Item orig_item;
  Item *item;
  int x;
  int y;
} ItemInputMode;

BaseInputMode base_input_mode;
ZoomInputMode zoom_input_mode;
PanInputMode pan_input_mode;
ItemInputMode item_input_mode;

#endif
