#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef USE_BACKTRACE
  #include <backtrace.h>
  struct backtrace_state *debug_backtrace_state = NULL;

  void debug_backtrace_print(int skip, FILE *fd) {
    if (!debug_backtrace_state) debug_backtrace_state = backtrace_create_state(NULL, 1, NULL, NULL);
    backtrace_print (debug_backtrace_state, skip + 1, fd);
  }
#else
  void debug_backtrace_print(int skip, FILE *fd) {}
#endif

FILE *eventlog = NULL;

int debug_enabled(int dfl, const char *prefix, const char *file, const char *func, const char *entry) {
  size_t prefix_len = strlen(prefix); 
  size_t file_len = strlen(file);
  size_t func_len = strlen(func);
  size_t entry_len = strlen(entry);
  char key[prefix_len + 1 + file_len + 1 + func_len + 1 + entry_len + 1];
  char *res;
  
  strcpy(key, prefix);
  key[prefix_len] = '_';
  strcpy(key + prefix_len + 1, file);
  key[prefix_len + 1 + file_len] = '_';
  strcpy(key + prefix_len + 1 + file_len + 1, func);
  key[prefix_len + 1 + file_len + 1 + func_len] = '_';
  strcpy(key + prefix_len + 1 + file_len + 1 + func_len + 1, entry);
  for (char *pos = key; *pos; pos++) {
    if (*pos == '.') {
      *pos = '_';
    }
  }

  res = getenv(key);
  if (res && res[0] == '1') return 1;
  if (res && res[0] == '0') return 0;

  for (char *pos = key + prefix_len + 1 + file_len + 1 + func_len + 1 + entry_len - 1;
       pos > key;
       pos--) {
    if (*pos == '_') {
      *pos = '\0';
      res = getenv(key);
      if (res && res[0] == '1') return 1;
      if (res && res[0] == '0') return 0;
    }
  }

  return dfl;
}

void debug_print(FILE *fd, int dfl, const char *prefix, const char *file, const char *func, const char *entry, const char * format, ...) {
  size_t prefix_len = strlen(prefix);
  size_t file_len = strlen(file);
  size_t func_len = strlen(func);
  size_t entry_len = strlen(entry);
  char key[prefix_len + 1 + file_len + 1 + func_len + 1 + entry_len + 1];
  va_list args;
  
  strcpy(key, prefix);
  key[prefix_len] = '.';
  strcpy(key + prefix_len + 1, file);
  key[prefix_len + 1 + file_len] = '.';
  strcpy(key + prefix_len + 1 + file_len + 1, func);
  key[prefix_len + 1 + file_len + 1 + func_len] = '.';
  strcpy(key + prefix_len + 1 + file_len + 1 + func_len + 1, entry);
  
  fprintf(fd, "%s: ", key);
  va_start(args, format);
  vfprintf(fd, format, args);
  va_end(args);
  fflush(fd);
}

char INDENT_STR[] = "                                                                ";
char *get_indent(int chars) {
  if (chars > sizeof(INDENT_STR) - 1) {
    chars = sizeof(INDENT_STR) - 1;
  }
  return INDENT_STR + (sizeof(INDENT_STR) - 1 - chars);
}
