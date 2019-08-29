#ifndef LIST
#define LIST

#include <string.h>

typedef struct {
  void **entries;
  size_t size;
  size_t count;
} List;

extern List *list_create();
extern void list_destroy(List *list);
extern void list_append(List *list, void *entry);
extern void list_remove(List *list, void *entry);
extern void *list_pop(List *list);

#endif
