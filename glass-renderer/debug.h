#ifndef DEBUG_H
#define DEBUG_H

extern int debug_enabled(const char *file, const char *func, const char *entry);
extern void debug_print(const char *file, const char *func, const char *entry, const char * format, ...);


#define DEBUG(entry, ...) debug_print(__FILE__, __func__, entry, __VA_ARGS__)
#define DEBUG_ENABLED(entry) debug_enabled(__FILE__, __func__, entry)

#endif
