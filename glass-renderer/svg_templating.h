/* SVG templating
 * This module resolves <use> tags in SVG documents. Inputs and output are SVG source as strings.
 * URL resolution and download is left to the caller (using svg_templating_list_urls and svg_templating_replace).
 * Recursive resolution, where a fragment for a <use> tag itself contains <use> tags, is supported,
 * as are multiple tags referring to the same URL as well as updating the contents for a URL multiple times.
 */
#ifndef SVG_TEMPLATING_H
#define SVG_TEMPLATING_H

#include <libxml/tree.h>

// Consider all struct fields read only
// Only use the provided API functions for updating them
typedef struct {
  char *url;
  xmlNodePtr g_node;
  void *data;
} BindingEntry;

typedef struct {
  xmlDocPtr doc;
  xmlNodePtr root;
  BindingEntry *bindings;
  int bindings_count;
} SvgTemplating;

SvgTemplating* svg_templating_create(const char *svg_content);
void svg_templating_free(SvgTemplating *mgr);

// Set the content for a <use> tag by its url
// data will be set on BindingEntry.data. Note that some other data
// strcuture must own this data as binding entries can be deleted at
// any time, and no callback is called to free this data. 
void svg_templating_replace_by_index(SvgTemplating *mgr, size_t index, const char *new_svg, void *data);
int svg_templating_replace_by_url(SvgTemplating *mgr, const char *url, const char *new_svg, void *data);
int svg_templating_replace_by_data(SvgTemplating *mgr, void *data, const char *new_svg);

// Remove invalidated bindings (call this once after calling the replace-functions above)
void svg_templating_gc(SvgTemplating *mgr);

// Renders the current tree as a SVG string
char* svg_templating_render(SvgTemplating *mgr);

#endif
