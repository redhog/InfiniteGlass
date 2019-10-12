#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

FILE *eventlog = NULL;

int debug_enabled(const char *prefix, const char *file, const char *func, const char *entry) {
  size_t prefix_len = strlen(prefix); 
  size_t file_len = strlen(file);
  size_t func_len = strlen(func);
  size_t entry_len = strlen(entry);
  char key[prefix_len + 1 + file_len + 1 + func_len + 1 + entry_len + 1];
  va_list args;
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

  return 0;
}

void debug_print(FILE *fd, const char *prefix, const char *file, const char *func, const char *entry, const char * format, ...) {
  if (!debug_enabled(prefix, file, func, entry)) return;

  size_t prefix_len = strlen(prefix);
  size_t file_len = strlen(file);
  size_t func_len = strlen(func);
  size_t entry_len = strlen(entry);
  char key[prefix_len + 1 + file_len + 1 + func_len + 1 + entry_len + 1];
  va_list args;
  int match = 0;
  
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


