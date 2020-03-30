#include "property_updater.h"
#include "mainloop.h"
#include "item.h"
#include "wm.h"
#include "xapi.h"
#include "view.h"
#include "concurrency.h"
#include "debug.h"
#include <unistd.h>
#include <ck_queue.h>

CK_STAILQ_HEAD(PropertyUpdaterQueue, PropertyUpdaterEntry) property_updater_queue = CK_STAILQ_HEAD_INITIALIZER(property_updater_queue);
struct PropertyUpdaterEntry {
  PropertyFetch *fetch;
  CK_STAILQ_ENTRY(PropertyUpdaterEntry) entries;
};


void *property_updater_main(void *arg) {
  XConnection *conn = xinit_basic();
  struct PropertyUpdaterEntry *entry;
  
  while (1) {
    while (!CK_STAILQ_EMPTY(&property_updater_queue)) {
      entry = CK_STAILQ_FIRST(&property_updater_queue);
      CK_STAILQ_REMOVE(&property_updater_queue, entry, PropertyUpdaterEntry, entries);

      property_fetch(conn, entry->fetch);

      WITH_LOCK(&global_lock) {
        Item *item = item_get_from_window(conn, entry->fetch->win, False);
        if (item) {
          if (item_properties_update(conn, item, entry->fetch)) {
            trigger_draw();
          }
        } else {
          XFree(entry->fetch->data);
        }
      }
      
      free(entry->fetch);
      free(entry);
    }
    usleep(100);
  }
  return NULL;
}

pthread_t property_updater_thread;

void property_updater(void) {  
  CK_STAILQ_INIT(&property_updater_queue);
  if (pthread_create(&property_updater_thread, NULL, &property_updater_main, NULL) != 0) {
    ERROR("create_thread", "Unable to create property updater thread.\n");
  }
}


void property_updater_submit(PropertyFetch *fetch) {
  struct PropertyUpdaterEntry *entry;
  entry = malloc(sizeof(struct PropertyUpdaterEntry));
  entry->fetch = fetch;
  CK_STAILQ_INSERT_HEAD(&property_updater_queue, entry, entries);
}
