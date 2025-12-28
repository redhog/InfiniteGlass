#include "svg_templating.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    const char *svg = "<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>\n"
                      "  <use xlink:href='#shape1' />\n"
                      "  <g id='1'>\n"
                      "    <use xlink:href='#shape1' />\n"
                      "  </g>\n"
                      "</svg>";

    SvgTemplating *mgr = svg_templating_create(svg);

    printf("References:\n");
    for(int i=0; i < mgr->bindings_count; i++){
      printf("URL: %s\n", mgr->bindings[i].url);
    }

    svg_templating_replace_by_url(mgr, "#shape1", "<use  xlink:href='#shape2' />\n", NULL);
    svg_templating_replace_by_url(mgr, "#shape2", "<rect width='10' height='10' fill='red'/>\n", NULL);
    svg_templating_replace_by_url(mgr, "#shape2", "<rect width='10' height='10' fill='green'/>\n", NULL);

    char *result = svg_templating_render(mgr);
    printf("Rendered SVG:\n%s\n", result);
    free(result);

    svg_templating_free(mgr);
}
