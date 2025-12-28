// svg_templating.c
#include "svg_templating.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <string.h>
#include <stdlib.h>

struct SvgTemplating {
    xmlDocPtr doc;
    xmlNodePtr root;
    struct LookupEntry {
        char *url;
        xmlNodePtr g_node;
    } *lookup;
    int lookup_count;
};

static void process_use_nodes(SvgTemplating *mgr) {
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(mgr->doc);
    xmlXPathRegisterNs(xpathCtx, (const xmlChar *)"svg", (const xmlChar *)"http://www.w3.org/2000/svg");
    xmlXPathRegisterNs(xpathCtx, (const xmlChar *)"xlink", (const xmlChar *)"http://www.w3.org/1999/xlink");
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)"//svg:use", xpathCtx);

    if(xpathObj && xpathObj->nodesetval) {
        int count = xpathObj->nodesetval->nodeNr;
        mgr->lookup = calloc(count, sizeof(*mgr->lookup));
        mgr->lookup_count = 0;

        for(int i = 0; i < count; i++) {
            xmlNodePtr use_node = xpathObj->nodesetval->nodeTab[i];
            xmlChar *href = xmlGetProp(use_node, (const xmlChar*)"xlink:href");
            if(!href) href = xmlGetProp(use_node, (const xmlChar*)"href");
            if(!href) continue;

            // Create a <g> node to replace <use>
            xmlNodePtr g_node = xmlNewNode(NULL, (const xmlChar*)"g");

            // Copy attributes (except href)
            for(xmlAttrPtr attr = use_node->properties; attr; attr = attr->next) {
                if(strcmp((const char*)attr->name, "xlink:href") != 0 && strcmp((const char*)attr->name, "href") != 0)
                    xmlSetProp(g_node, attr->name, attr->children->content);
            }

            xmlReplaceNode(use_node, g_node);
            xmlFreeNode(use_node);

            mgr->lookup[mgr->lookup_count].url = strdup((const char*)href);
            mgr->lookup[mgr->lookup_count].g_node = g_node;
            mgr->lookup_count++;

            xmlFree(href);
        }
    }
    if(xpathObj) xmlXPathFreeObject(xpathObj);
    if(xpathCtx) xmlXPathFreeContext(xpathCtx);
}

SvgTemplating* svg_templating_create(const char *svg_content) {
    xmlDocPtr doc = xmlParseMemory(svg_content, strlen(svg_content));
    if(!doc) return NULL;

    SvgTemplating *mgr = calloc(1, sizeof(SvgTemplating));
    mgr->doc = doc;
    mgr->root = xmlDocGetRootElement(doc);

    process_use_nodes(mgr);
    return mgr;
}

void svg_templating_free(SvgTemplating *mgr) {
    if(!mgr) return;
    for(int i = 0; i < mgr->lookup_count; i++) {
        free(mgr->lookup[i].url);
    }
    free(mgr->lookup);
    if(mgr->doc) xmlFreeDoc(mgr->doc);
    free(mgr);
}

char** svg_templating_list_urls(SvgTemplating *mgr) {
    char **urls = calloc(mgr->lookup_count + 1, sizeof(char*));
    for(int i = 0; i < mgr->lookup_count; i++)
        urls[i] = strdup(mgr->lookup[i].url);
    urls[mgr->lookup_count] = NULL;
    return urls;
}

static void lookup_add(SvgTemplating *mgr, const char *url, xmlNodePtr g_node) {
    mgr->lookup = realloc(mgr->lookup, sizeof(*mgr->lookup) * (mgr->lookup_count + 1));
    mgr->lookup[mgr->lookup_count].url = strdup(url);
    mgr->lookup[mgr->lookup_count].g_node = g_node;
    mgr->lookup_count++;
}

static void lookup_compact(SvgTemplating *mgr) {
    int wr = 0;
    for (int i = 0; i < mgr->lookup_count; i++) {
        if (mgr->lookup[i].g_node && mgr->lookup[i].g_node->parent) {
            mgr->lookup[wr++] = mgr->lookup[i];
        } else {
            free(mgr->lookup[i].url);
        }
    }
    mgr->lookup_count = wr;
    mgr->lookup = realloc(mgr->lookup, sizeof(*mgr->lookup) * mgr->lookup_count);
}

static void scan_use_nodes_recursive(SvgTemplating *mgr, xmlNodePtr parent) {
    for (xmlNodePtr cur = parent->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE && xmlStrEqual(cur->name, (xmlChar*)"use")) {

            xmlChar *href = xmlGetProp(cur, (xmlChar*)"xlink:href");
            if (!href) href = xmlGetProp(cur, (xmlChar*)"href");
            if (!href) continue;

            xmlNodePtr g_node = xmlNewNode(NULL, (xmlChar*)"g");

            for (xmlAttrPtr attr = cur->properties; attr; attr = attr->next) {
                if (xmlStrcmp(attr->name, (xmlChar*)"xlink:href") != 0 &&
                    xmlStrcmp(attr->name, (xmlChar*)"href") != 0) {
                    xmlSetProp(g_node, attr->name, attr->children->content);
                }
            }

            xmlReplaceNode(cur, g_node);
            xmlFreeNode(cur);

            lookup_add(mgr, (char*)href, g_node);
            xmlFree(href);

            // recurse into this new g-node
            scan_use_nodes_recursive(mgr, g_node);
        }

        // Recurse into children normally
        if (cur->children)
            scan_use_nodes_recursive(mgr, cur);
    }
}

int svg_templating_replace(SvgTemplating *mgr, const char *url, const char *new_svg) {
    int found = 0;

    for (int i = 0; i < mgr->lookup_count; i++) {
        if (strcmp(mgr->lookup[i].url, url) == 0) {
            found = 1;
            xmlNodePtr g = mgr->lookup[i].g_node;

            // parse wrapped fragment
            char *wrapped = malloc(strlen(new_svg) + 20);
            sprintf(wrapped, "<root>%s</root>", new_svg);
            xmlDocPtr tmp = xmlParseMemory(wrapped, strlen(wrapped));
            free(wrapped);
            if (!tmp) continue;
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

            // recursively scan newly added nodes, add more lookup entries
            scan_use_nodes_recursive(mgr, g);
        }
    }

    // cleanup dead entries now (AFTER replacing all matches)
    lookup_compact(mgr);

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
