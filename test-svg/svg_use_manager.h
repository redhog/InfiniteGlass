// svg_use_manager.h
#ifndef SVG_USE_MANAGER_H
#define SVG_USE_MANAGER_H

#include <libxml/tree.h>

typedef struct SvgUseManager SvgUseManager;

SvgUseManager* svg_use_manager_create(const char *svg_content);
void svg_use_manager_free(SvgUseManager *mgr);

// Returns a NULL-terminated array of strings containing the URLs
char** svg_use_manager_list_urls(SvgUseManager *mgr);

// Replaces the content of a <g> corresponding to a URL with the content
// of a new SVG string
int svg_use_manager_replace(SvgUseManager *mgr, const char *url, const char *new_svg);

// Renders the current tree as a string
char* svg_use_manager_render(SvgUseManager *mgr);

#endif
