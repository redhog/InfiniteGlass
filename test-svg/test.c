#include "svg_use_manager.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    const char *svg = "<svg xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>\n"
                      "  <use xlink:href='#shape1' />\n"
                      "  <g id='1'>\n"
                      "    <use xlink:href='#shape1' />\n"
                      "  </g>\n"
                      "</svg>";

    SvgUseManager *mgr = svg_use_manager_create(svg);

    printf("References:\n");
    char **urls = svg_use_manager_list_urls(mgr);
    for(int i=0; urls[i]; i++){
        printf("URL: %s\n", urls[i]);
        free(urls[i]);
    }
    free(urls);

    svg_use_manager_replace(mgr, "#shape1", "<use  xlink:href='#shape2' />\n");
    svg_use_manager_replace(mgr, "#shape2", "<rect width='10' height='10' fill='red'/>\n");
    svg_use_manager_replace(mgr, "#shape2", "<rect width='10' height='10' fill='green'/>\n");

    char *result = svg_use_manager_render(mgr);
    printf("Rendered SVG:\n%s\n", result);
    free(result);

    svg_use_manager_free(mgr);
}
