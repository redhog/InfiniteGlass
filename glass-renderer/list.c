#include "list.h"
#include <string.h>
#include <stdlib.h>

List *list_create() {
  List *list = malloc(sizeof(List));
  list->entries = NULL;
  list->count = 0;
  list->size = 0;
  return list;
}

void list_destroy(List *list) {
  free(list->entries);
  free(list);
}

void list_append(List *list, void *entry) {
  if (list->count+1 >= list->size) {
    list->size = list->count + 32;
    list->entries = realloc(list->entries, list->size * sizeof(void *));
  }
  list->entries[list->count] = entry;
  list->count++; 
}

void list_remove(List *list, void *entry) {
  size_t idx;
  for (idx = 0; idx < list->count && list->entries[idx] != entry; idx++);
  if (idx >= list->count) {
    *(char *) NULL = 0;
  }
  memmove(&list->entries[idx], &list->entries[idx+1], sizeof(void *) * (list->count - idx - 1));
  list->count -= 1;
}

void *list_pop(List *list) {
  list->count -= 1;
  return list->entries[list->count];
}


