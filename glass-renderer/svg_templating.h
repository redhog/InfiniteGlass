/* SVG templating
 * This module resolves <use> tags in SVG documents. Inputs and output are SVG source as strings.
 * URL resolution and download is left to the caller (using svg_templating_list_urls and svg_templating_replace).
 * Recursive resolution, where a fragment for a <use> tag itself contains <use> tags, is supported,
 * as are multiple tags referring to the same URL as well as updating the contents for a URL multiple times.
 */
#ifndef SVG_TEMPLATING_H
#define SVG_TEMPLATING_H

#include <libxml/tree.h>

typedef struct SvgTemplating SvgTemplating;

SvgTemplating* svg_templating_create(const char *svg_content);
void svg_templating_free(SvgTemplating *mgr);

// Returns a NULL-terminated array of strings containing the URLs from <use> tags
char** svg_templating_list_urls(SvgTemplating *mgr);

// Set the content for a <use> tag by its url
int svg_templating_replace(SvgTemplating *mgr, const char *url, const char *new_svg);

// Renders the current tree as a SVG string
char* svg_templating_render(SvgTemplating *mgr);

#endif
