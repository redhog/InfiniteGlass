// svg_templating.c
#include "svg_templating.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string.h>
#include <stdlib.h>


static int parent_is_text_container(xmlNodePtr parent) {
    if (!parent || parent->type != XML_ELEMENT_NODE)
        return 0;

    return xmlStrEqual(parent->name, (xmlChar*)"text") ||
           xmlStrEqual(parent->name, (xmlChar*)"tspan") ||
           xmlStrEqual(parent->name, (xmlChar*)"textPath");
}

static void process_use_nodes(SvgTemplating *mgr) {
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(mgr->doc);
    xmlXPathRegisterNs(xpathCtx, (const xmlChar *)"svg",
                      (const xmlChar *)"http://www.w3.org/2000/svg");
    xmlXPathRegisterNs(xpathCtx, (const xmlChar *)"xlink",
                      (const xmlChar *)"http://www.w3.org/1999/xlink");

    xmlXPathObjectPtr xpathObj =
        xmlXPathEvalExpression((const xmlChar*)"//svg:use", xpathCtx);

    if (xpathObj && xpathObj->nodesetval) {
        int count = xpathObj->nodesetval->nodeNr;
        mgr->bindings = calloc(count, sizeof(*mgr->bindings));
        mgr->bindings_count = 0;

        for (int i = 0; i < count; i++) {
            xmlNodePtr use_node = xpathObj->nodesetval->nodeTab[i];

            xmlChar *href = xmlGetProp(use_node, (const xmlChar*)"xlink:href");
            if (!href)
                href = xmlGetProp(use_node, (const xmlChar*)"href");
            if (!href)
                continue;

            xmlNodePtr parent = use_node->parent;

            /* Decide replacement element */
            const xmlChar *new_name =
                parent_is_text_container(parent)
                ? (const xmlChar*)"tspan"
                : (const xmlChar*)"g";

            xmlNodePtr new_node = xmlNewNode(NULL, new_name);

            /* Copy attributes except href */
            for (xmlAttrPtr attr = use_node->properties; attr; attr = attr->next) {
                if (!xmlStrEqual(attr->name, (xmlChar*)"xlink:href") &&
                    !xmlStrEqual(attr->name, (xmlChar*)"href")) {

                    xmlChar *value = xmlNodeGetContent((xmlNodePtr)attr);
                    if (value) {
                        xmlSetProp(new_node, attr->name, value);
                        xmlFree(value);
                    }
                }
            }

            xmlReplaceNode(use_node, new_node);
            xmlFreeNode(use_node);

            mgr->bindings[mgr->bindings_count].url =
                strdup((const char*)href);
            mgr->bindings[mgr->bindings_count].g_node = new_node;
            mgr->bindings_count++;

            xmlFree(href);
        }
    }

    if (xpathObj)
        xmlXPathFreeObject(xpathObj);
    if (xpathCtx)
        xmlXPathFreeContext(xpathCtx);
}

SvgTemplating* svg_templating_create_from_bytes(const char *svg_content, size_t len) {
    xmlDocPtr doc = xmlParseMemory(svg_content, len);
    if(!doc) return NULL;

    SvgTemplating *mgr = calloc(1, sizeof(SvgTemplating));
    mgr->doc = doc;
    mgr->root = xmlDocGetRootElement(doc);

    process_use_nodes(mgr);
    return mgr;
}

SvgTemplating* svg_templating_create(const char *svg_content) {
    return svg_templating_create_from_bytes(svg_content, strlen(svg_content));
}

void svg_templating_free(SvgTemplating *mgr) {
    if(!mgr) return;
    for(int i = 0; i < mgr->bindings_count; i++) {
        free(mgr->bindings[i].url);
    }
    free(mgr->bindings);
    if(mgr->doc) xmlFreeDoc(mgr->doc);
    free(mgr);
}

static void bindings_add(SvgTemplating *mgr, const char *url, xmlNodePtr g_node) {
    mgr->bindings = realloc(mgr->bindings, sizeof(*mgr->bindings) * (mgr->bindings_count + 1));
    mgr->bindings[mgr->bindings_count].url = strdup(url);
    mgr->bindings[mgr->bindings_count].g_node = g_node;
    mgr->bindings[mgr->bindings_count].data = NULL;
    mgr->bindings_count++;
}

void svg_templating_gc(SvgTemplating *mgr) {
    int wr = 0;
    for (int i = 0; i < mgr->bindings_count; i++) {
        if (mgr->bindings[i].g_node && mgr->bindings[i].g_node->parent) {
            if (wr != i) {
                mgr->bindings[wr] = mgr->bindings[i];
            }
            wr++;
        } else {
            free(mgr->bindings[i].url);
        }
    }

    mgr->bindings_count = wr;

    if (wr > 0) {
        void *tmp = realloc(mgr->bindings, sizeof(*mgr->bindings) * wr);
        if (tmp) {
            mgr->bindings = tmp;
        } else {
            // realloc failed, we keep old pointer (no shrink)
        }
    } else {
        free(mgr->bindings);
        mgr->bindings = NULL;
    }
}

static void scan_use_nodes_recursive(SvgTemplating *mgr, xmlNodePtr parent) {
    for (xmlNodePtr cur = parent->children; cur; ) {
        xmlNodePtr next = cur->next;  // save next because cur may be replaced

        if (cur->type == XML_ELEMENT_NODE &&
            xmlStrEqual(cur->name, (xmlChar*)"use")) {

            xmlChar *href = xmlGetProp(cur, (xmlChar*)"xlink:href");
            if (!href)
                href = xmlGetProp(cur, (xmlChar*)"href");
            if (!href) {
                cur = next;
                continue;
            }

            // Decide replacement element
            const xmlChar *new_name =
                parent_is_text_container(parent)
                ? (xmlChar*)"tspan"
                : (xmlChar*)"g";

            xmlNodePtr new_node = xmlNewNode(NULL, new_name);

            // Copy attributes except href
            for (xmlAttrPtr attr = cur->properties; attr; attr = attr->next) {
                if (!xmlStrEqual(attr->name, (xmlChar*)"xlink:href") &&
                    !xmlStrEqual(attr->name, (xmlChar*)"href")) {

                    xmlChar *value = xmlNodeGetContent((xmlNodePtr)attr);
                    if (value) {
                        xmlSetProp(new_node, attr->name, value);
                        xmlFree(value);
                    }
                }
            }

            xmlReplaceNode(cur, new_node);
            xmlFreeNode(cur);

            bindings_add(mgr, (char*)href, new_node);
            xmlFree(href);

            // Recurse into replacement node
            scan_use_nodes_recursive(mgr, new_node);
        } else if (cur->children) {
            scan_use_nodes_recursive(mgr, cur);
        }

        cur = next;
    }
}

void svg_templating_replace_by_index(SvgTemplating *mgr, size_t index, const char *new_svg, void *data) {
    svg_templating_replace_by_index_from_bytes(mgr, index, new_svg, strlen(new_svg), data);
}

void svg_templating_replace_by_index_from_bytes(SvgTemplating *mgr, size_t index, const char *new_svg, size_t new_svg_len, void *data) {
    xmlNodePtr g = mgr->bindings[index].g_node;
    mgr->bindings[index].data = data;
    
    // parse wrapped fragment
    char *wrapper = "<root xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>%.*s</root>";
    char *wrapped = malloc(new_svg_len + strlen(wrapper) + 1);
    sprintf(wrapped, wrapper, (int)new_svg_len, new_svg);
    xmlDocPtr tmp = xmlParseMemory(wrapped, strlen(wrapped));
    free(wrapped);
    if (!tmp) return;
    xmlNodePtr tmp_root = xmlDocGetRootElement(tmp);

    // delete all old children
    xmlNodePtr c, nxt;
    for (c = g->children; c; c = nxt) {
        nxt = c->next;
        xmlUnlinkNode(c);
        xmlFreeNode(c);
    }

    // import new children
    for (c = tmp_root->children; c; c = c->next) {
        xmlNodePtr imported = xmlDocCopyNode(c, mgr->doc, 1);
        xmlAddChild(g, imported);
    }
    xmlFreeDoc(tmp);

    // recursively scan newly added nodes, add more bindings entries
    scan_use_nodes_recursive(mgr, g);
}

int svg_templating_replace_by_url(SvgTemplating *mgr, const char *url, const char *new_svg, void *data) {
    int found = 0;
    for (int i = 0; i < mgr->bindings_count; i++) {
        if (strcmp(mgr->bindings[i].url, url) == 0) {
            found = 1;
            svg_templating_replace_by_index(mgr, i, new_svg, data);
        }
    }
    return found ? 0 : -1;
}

int svg_templating_replace_by_data(SvgTemplating *mgr, void *data, const char *new_svg) {
    return svg_templating_replace_by_data_from_bytes(mgr, data, new_svg, strlen(new_svg));
}

int svg_templating_replace_by_data_from_bytes(SvgTemplating *mgr, void *data, const char *new_svg, size_t new_svg_len) {
    int found = 0;
    for (int i = 0; i < mgr->bindings_count; i++) {
        if (mgr->bindings[i].data == data) {
            found = 1;
            svg_templating_replace_by_index_from_bytes(mgr, i, new_svg, new_svg_len, data);
        }
    }
    return found ? 0 : -1;
}

char* svg_templating_render(SvgTemplating *mgr) {
    xmlChar *xmlbuff;
    int buffersize;
    xmlDocDumpFormatMemoryEnc(mgr->doc, &xmlbuff, &buffersize, "UTF-8", 1);
    char *result = strndup((const char*)xmlbuff, buffersize);
    xmlFree(xmlbuff);
    return result;
}
